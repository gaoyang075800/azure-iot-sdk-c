// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <ctype.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/httpapiexsas.h"
#include "azure_c_shared_utility/base64.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/connection_string_parser.h"

#include "parson.h"
#include "iothub_deviceconfiguration.h"
#include "iothub_sc_version.h"

#define IOTHUB_DEVICECONFIGURATION_REQUEST_MODE_VALUES  \
    IOTHUB_DEVICECONFIGURATION_REQUEST_GET_LIST,             \
    IOTHUB_DEVICECONFIGURATION_REQUEST_GET,      \
    IOTHUB_DEVICECONFIGURATION_REQUEST_ADD,             \
    IOTHUB_DEVICECONFIGURATION_REQUEST_UPDATE,          \
    IOTHUB_DEVICECONFIGURATION_REQUEST_DELETE,          \
    IOTHUB_DEVICECONFIGURATION_REQUEST_TESTQUERIES

DEFINE_ENUM(IOTHUB_DEVICECONFIGURATION_REQUEST_MODE, IOTHUB_DEVICECONFIGURATION_REQUEST_MODE_VALUES);
DEFINE_ENUM_STRINGS(IOTHUB_DEVICE_CONFIGURATION_RESULT, IOTHUB_DEVICE_CONFIGURATION_RESULT_VALUES);

#define  HTTP_HEADER_KEY_AUTHORIZATION  "Authorization"
#define  HTTP_HEADER_VAL_AUTHORIZATION  " "
#define  HTTP_HEADER_KEY_REQUEST_ID  "Request-Id"
#define  HTTP_HEADER_KEY_USER_AGENT  "User-Agent"
#define  HTTP_HEADER_VAL_USER_AGENT  IOTHUB_SERVICE_CLIENT_TYPE_PREFIX IOTHUB_SERVICE_CLIENT_BACKSLASH IOTHUB_SERVICE_CLIENT_VERSION
#define  HTTP_HEADER_KEY_ACCEPT  "Accept"
#define  HTTP_HEADER_VAL_ACCEPT  "application/json"
#define  HTTP_HEADER_KEY_CONTENT_TYPE  "Content-Type"
#define  HTTP_HEADER_VAL_CONTENT_TYPE  "application/json; charset=utf-8"
#define  HTTP_HEADER_KEY_IFMATCH  "If-Match"
#define  HTTP_HEADER_VAL_IFMATCH  "'*'"
#define UID_LENGTH 37

static const char* const URL_API_VERSION = "api-version=2018-03-01-preview";

static const char* const RELATIVE_PATH_FMT_DEVICECONFIGURATION = "/configurations/%s?%s";
static const char* const RELATIVE_PATH_FMT_DEVICECONFIGURATIONS = "/configurations/?top=%s&%s";
static const char* const RELATIVE_PATH_FMT_DEVICECONFIGURATION_TESTQUERIES = "/configurationstestQueries?%s";
static const char* const RELATIVE_PATH_FMT_DEVICECONFIGURATION_PAYLOAD = "{\"methodName\":\"%s\",\"timeout\":%d,\"payload\":%s}";

//TODO: add this to devices API
//static const char* const RELATIVE_PATH_FMT_APPLY_DEVICECONFIGURATION = "/devices/%s/applyConfigurationContent?";

typedef struct IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_TAG
{
    char* hostname;
    char* sharedAccessKey;
    char* keyName;
} IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION;

static const char* generateGuid(void)
{
    char* result;

    if ((result = malloc(UID_LENGTH)) != NULL)
    {
        result[0] = '\0';
        if (UniqueId_Generate(result, UID_LENGTH) != UNIQUEID_OK)
        {
            free((void*)result);
            result = NULL;
        }
    }
    return (const char*)result;
}

static STRING_HANDLE createRelativePath(IOTHUB_DEVICECONFIGURATION_REQUEST_MODE iotHubDeviceConfigurationRequestMode, const char* configurationId)
{
    //IOTHUB_DEVICECONFIGURATION_REQUEST_GET            GET      {iot hub}/configurations/{configuration id}          // Get single device configuration
    //IOTHUB_DEVICECONFIGURATION_REQUEST_ADD            PUT      {iot hub}/configurations/{configuration id}          // Add device configuration
    //IOTHUB_DEVICECONFIGURATION_REQUEST_UPDATE         PUT      {iot hub}/configurations/{configuration id}          // Update device configuration
    //IOTHUB_DEVICECONFIGURATION_REQUEST_DELETE         DELETE   {iot hub}/configurations/{configuration id}          // Delete device configuration
    //IOTHUB_DEVICECONFIGURATION_REQUEST_GET_LIST       GET      {iot hub}/configurations                             // Get multiple configurations
    //IOTHUB_DEVICECONFIGURATION_REQUEST_TESTQUERIES    POST     {iot hub}/configurations/testQueries                 // Test configuration queries

    STRING_HANDLE result;

    if (iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_GET_LIST)
    {
        result = STRING_construct_sprintf(RELATIVE_PATH_FMT_DEVICECONFIGURATIONS, URL_API_VERSION);
    }
    else if ((iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_ADD) || (iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_UPDATE) || (iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_GET) || (iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_DELETE))
    {
        result = STRING_construct_sprintf(RELATIVE_PATH_FMT_DEVICECONFIGURATION, configurationId, URL_API_VERSION);
    }
    else if (iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_TESTQUERIES)
    {
        result = STRING_construct_sprintf(RELATIVE_PATH_FMT_DEVICECONFIGURATION_TESTQUERIES, URL_API_VERSION);
    }
    else
    {
        result = NULL;
    }
    return result;
}

static HTTP_HEADERS_HANDLE createHttpHeader(IOTHUB_DEVICECONFIGURATION_REQUEST_MODE iotHubDeviceConfigurationRequestMode)
{
    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_020: [ IoTHubDeviceConfiguration_GetConfiguration shall add the following headers to the created HTTP GET request: authorization=sasToken,Request-Id=1001,Accept=application/json,Content-Type=application/json,charset=utf-8 ]*/
    HTTP_HEADERS_HANDLE httpHeader;
    const char* guid = NULL;

    if ((httpHeader = HTTPHeaders_Alloc()) == NULL)
    {
        LogError("HTTPHeaders_Alloc failed");
    }
    else if (HTTPHeaders_AddHeaderNameValuePair(httpHeader, HTTP_HEADER_KEY_AUTHORIZATION, HTTP_HEADER_VAL_AUTHORIZATION) != HTTP_HEADERS_OK)
    {
        LogError("HTTPHeaders_AddHeaderNameValuePair failed for Authorization header");
        HTTPHeaders_Free(httpHeader);
        httpHeader = NULL;
    }
    else if ((guid = generateGuid()) == NULL)
    {
        LogError("GUID creation failed");
        HTTPHeaders_Free(httpHeader);
        httpHeader = NULL;
    }
    else if (HTTPHeaders_AddHeaderNameValuePair(httpHeader, HTTP_HEADER_KEY_REQUEST_ID, guid) != HTTP_HEADERS_OK)
    {
        LogError("HTTPHeaders_AddHeaderNameValuePair failed for RequestId header");
        HTTPHeaders_Free(httpHeader);
        httpHeader = NULL;
    }
    else if (HTTPHeaders_AddHeaderNameValuePair(httpHeader, HTTP_HEADER_KEY_USER_AGENT, HTTP_HEADER_VAL_USER_AGENT) != HTTP_HEADERS_OK)
    {
        LogError("HTTPHeaders_AddHeaderNameValuePair failed for User-Agent header");
        HTTPHeaders_Free(httpHeader);
        httpHeader = NULL;
    }
    else if (HTTPHeaders_AddHeaderNameValuePair(httpHeader, HTTP_HEADER_KEY_CONTENT_TYPE, HTTP_HEADER_VAL_CONTENT_TYPE) != HTTP_HEADERS_OK)
    {
        LogError("HTTPHeaders_AddHeaderNameValuePair failed for Content-Type header");
        HTTPHeaders_Free(httpHeader);
        httpHeader = NULL;
    }
    else if ((iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_ADD) || (iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_UPDATE) || (iotHubDeviceConfigurationRequestMode != IOTHUB_DEVICECONFIGURATION_REQUEST_DELETE))
    {
        if (HTTPHeaders_AddHeaderNameValuePair(httpHeader, HTTP_HEADER_KEY_IFMATCH, HTTP_HEADER_VAL_IFMATCH) != HTTP_HEADERS_OK)
        {
            LogError("HTTPHeaders_AddHeaderNameValuePair failed for If-Match header");
            HTTPHeaders_Free(httpHeader);
            httpHeader = NULL;
        }
    }
    free((void*)guid);

    return httpHeader;
}

static IOTHUB_DEVICE_CONFIGURATION_RESULT sendHttpRequestDeviceConfiguration(IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE serviceClientDeviceConfigurationHandle, IOTHUB_DEVICECONFIGURATION_REQUEST_MODE iotHubDeviceConfigurationRequestMode, const char* configurationId, BUFFER_HANDLE deviceJsonBuffer, BUFFER_HANDLE responseBuffer)
{
    IOTHUB_DEVICE_CONFIGURATION_RESULT result;

    STRING_HANDLE uriResource = NULL;
    STRING_HANDLE accessKey = NULL;
    STRING_HANDLE keyName = NULL;
    HTTPAPIEX_SAS_HANDLE httpExApiSasHandle;
    HTTPAPIEX_HANDLE httpExApiHandle;
    HTTP_HEADERS_HANDLE httpHeader;

    if ((uriResource = STRING_construct(serviceClientDeviceConfigurationHandle->hostname)) == NULL)
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_024: [ If any of the HTTPAPI call fails IoTHubDeviceConfiguration_GetConfiguration shall fail and return NULL ]*/
        LogError("STRING_construct failed for uriResource");
        result = IOTHUB_DEVICE_CONFIGURATION_ERROR;
    }
    else if ((accessKey = STRING_construct(serviceClientDeviceConfigurationHandle->sharedAccessKey)) == NULL)
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_024: [ If any of the call fails during the HTTP creation IoTHubDeviceConfiguration_GetConfiguration shall fail and return NULL ]*/
        LogError("STRING_construct failed for accessKey");
        STRING_delete(uriResource);
        result = IOTHUB_DEVICE_CONFIGURATION_ERROR;
    }
    else if ((keyName = STRING_construct(serviceClientDeviceConfigurationHandle->keyName)) == NULL)
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_024: [ If any of the call fails during the HTTP creation IoTHubDeviceConfiguration_GetConfiguration shall fail and return NULL ]*/
        LogError("STRING_construct failed for keyName");
        STRING_delete(accessKey);
        STRING_delete(uriResource);
        result = IOTHUB_DEVICE_CONFIGURATION_ERROR;
    }
    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_020: [ IoTHubDeviceConfiguration_GetConfiguration shall add the following headers to the created HTTP GET request: authorization=sasToken,Request-Id=1001,Accept=application/json,Content-Type=application/json,charset=utf-8 ]*/
    else if ((httpHeader = createHttpHeader(iotHubDeviceConfigurationRequestMode)) == NULL)
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_024: [ If any of the call fails during the HTTP creation IoTHubDeviceConfiguration_GetConfiguration shall fail and return NULL ]*/
        LogError("HttpHeader creation failed");
        STRING_delete(keyName);
        STRING_delete(accessKey);
        STRING_delete(uriResource);
        result = IOTHUB_DEVICE_CONFIGURATION_ERROR;
    }
    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_021: [ IoTHubDeviceConfiguration_GetConfiguration shall create an HTTPAPIEX_SAS_HANDLE handle by calling HTTPAPIEX_SAS_Create ]*/
    else if ((httpExApiSasHandle = HTTPAPIEX_SAS_Create(accessKey, uriResource, keyName)) == NULL)
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_025: [ If any of the HTTPAPI call fails IoTHubDeviceConfiguration_GetConfiguration shall fail and return IOTHUB_DEVICE_CONFIGURATION_HTTPAPI_ERROR ]*/
        LogError("HTTPAPIEX_SAS_Create failed");
        HTTPHeaders_Free(httpHeader);
        STRING_delete(keyName);
        STRING_delete(accessKey);
        STRING_delete(uriResource);
        result = IOTHUB_DEVICE_CONFIGURATION_HTTPAPI_ERROR;
    }
    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_022: [ IoTHubDeviceConfiguration_GetConfiguration shall create an HTTPAPIEX_HANDLE handle by calling HTTPAPIEX_Create ]*/
    else if ((httpExApiHandle = HTTPAPIEX_Create(serviceClientDeviceConfigurationHandle->hostname)) == NULL)
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_025: [ If any of the HTTPAPI call fails IoTHubDeviceConfiguration_GetConfiguration shall fail and return NULL ]*/
        LogError("HTTPAPIEX_Create failed");
        HTTPAPIEX_SAS_Destroy(httpExApiSasHandle);
        HTTPHeaders_Free(httpHeader);
        STRING_delete(keyName);
        STRING_delete(accessKey);
        STRING_delete(uriResource);
        result = IOTHUB_DEVICE_CONFIGURATION_HTTPAPI_ERROR;
    }
    else
    {
        HTTPAPI_REQUEST_TYPE httpApiRequestType = HTTPAPI_REQUEST_GET;
        STRING_HANDLE relativePath;
        unsigned int statusCode = 0;
        unsigned char is_error = 0;

        //IOTHUB_DEVICECONFIGURATION_REQUEST_GET            GET      {iot hub}/configurations/{configuration id}          // Get single device configuration
        //IOTHUB_DEVICECONFIGURATION_REQUEST_ADD            PUT      {iot hub}/configurations/{configuration id}          // Add device configuration
        //IOTHUB_DEVICECONFIGURATION_REQUEST_UPDATE         PUT      {iot hub}/configurations/{configuration id}          // Update device configuration
        //IOTHUB_DEVICECONFIGURATION_REQUEST_DELETE         DELETE   {iot hub}/configurations/{configuration id}          // Delete device configuration
        //IOTHUB_DEVICECONFIGURATION_REQUEST_GET_LIST       GET      {iot hub}/configurations                             // Get multiple configurations
        //IOTHUB_DEVICECONFIGURATION_REQUEST_TESTQUERIES    POST     {iot hub}/configurations/testQueries                 // Test configuration queries

        if ((iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_ADD) || (iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_UPDATE))
        {
            httpApiRequestType = HTTPAPI_REQUEST_PUT;
        }
        else if ((iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_TESTQUERIES))
        {
            httpApiRequestType = HTTPAPI_REQUEST_POST;
        }
        else if ((iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_GET) || (iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_GET_LIST))
        {
            httpApiRequestType = HTTPAPI_REQUEST_GET;
        }
        else if ((iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_DELETE))
        {
            httpApiRequestType = HTTPAPI_REQUEST_DELETE;
        }
        else
        {
            is_error = 1;
        }

        if (is_error)
        {
            LogError("Invalid request type");
            result = IOTHUB_DEVICE_CONFIGURATION_HTTPAPI_ERROR;
        }
        else
        {
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_019: [ IoTHubDeviceConfiguration_GetConfiguration shall create HTTP GET request URL using the given deviceId using the following format: url/configurations/[deviceId] ]*/
            if ((relativePath = createRelativePath(iotHubDeviceConfigurationRequestMode, configurationId)) == NULL)
            {
                /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_024: [ If any of the call fails during the HTTP creation IoTHubDeviceConfiguration_GetConfiguration shall fail and return NULL ]*/
                LogError("Failure creating relative path");
                result = IOTHUB_DEVICE_CONFIGURATION_ERROR;
            }
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ IoTHubDeviceConfiguration_GetConfiguration shall execute the HTTP GET request by calling HTTPAPIEX_ExecuteRequest ]*/
            else if (HTTPAPIEX_SAS_ExecuteRequest(httpExApiSasHandle, httpExApiHandle, httpApiRequestType, STRING_c_str(relativePath), httpHeader, deviceJsonBuffer, &statusCode, NULL, responseBuffer) != HTTPAPIEX_OK)
            {
                /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_025: [ If any of the HTTPAPI call fails IoTHubDeviceConfiguration_GetConfiguration shall fail and return NULL ]*/
                LogError("HTTPAPIEX_SAS_ExecuteRequest failed");
                STRING_delete(relativePath);
                result = IOTHUB_DEVICE_CONFIGURATION_HTTPAPI_ERROR;
            }
            else
            {
                STRING_delete(relativePath);
                if (statusCode == 200)
                {
                    /*CodesSRS_IOTHUBDEVICECONFIGURATION_01_030: [ Otherwise IoTHubDeviceConfiguration_GetConfiguration shall save the received deviceConfiguration to the out parameter and return with it ]*/
                    result = IOTHUB_DEVICE_CONFIGURATION_OK;
                }
                else
                {
                    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_026: [ IoTHubDeviceConfiguration_GetConfiguration shall verify the received HTTP status code and if it is not equal to 200 then return NULL ]*/
                    LogError("Http Failure status code %d.", statusCode);
                    result = IOTHUB_DEVICE_CONFIGURATION_ERROR;
                }
            }
        }
        HTTPAPIEX_Destroy(httpExApiHandle);
        HTTPAPIEX_SAS_Destroy(httpExApiSasHandle);
        HTTPHeaders_Free(httpHeader);
        STRING_delete(keyName);
        STRING_delete(accessKey);
        STRING_delete(uriResource);
    }
    return result;
}

static void free_deviceConfiguration_handle(IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION* deviceConfiguration)
{
    free(deviceConfiguration->hostname);
    free(deviceConfiguration->sharedAccessKey);
    free(deviceConfiguration->keyName);
    free(deviceConfiguration);
}

IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE IoTHubDeviceConfiguration_Create(IOTHUB_SERVICE_CLIENT_AUTH_HANDLE serviceClientHandle)
{
    IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE result;

    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_001: [ If the serviceClientHandle input parameter is NULL IoTHubDeviceConfiguration_Create shall return NULL ]*/
    if (serviceClientHandle == NULL)
    {
        LogError("IotHubDeviceConfiguration_Create: serviceClientHandle is null");
        result = NULL;
    }
    else
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_002: [ If any member of the serviceClientHandle input parameter is NULL IoTHubDeviceConfiguration_Create shall return NULL ]*/
        IOTHUB_SERVICE_CLIENT_AUTH* serviceClientAuth = (IOTHUB_SERVICE_CLIENT_AUTH*)serviceClientHandle;

        if (serviceClientAuth->hostname == NULL)
        {
            LogError("authInfo->hostName input parameter cannot be NULL");
            result = NULL;
        }
        else if (serviceClientAuth->iothubName == NULL)
        {
            LogError("authInfo->iothubName input parameter cannot be NULL");
            result = NULL;
        }
        else if (serviceClientAuth->iothubSuffix == NULL)
        {
            LogError("authInfo->iothubSuffix input parameter cannot be NULL");
            result = NULL;
        }
        else if (serviceClientAuth->keyName == NULL)
        {
            LogError("authInfo->keyName input parameter cannot be NULL");
            result = NULL;
        }
        else if (serviceClientAuth->sharedAccessKey == NULL)
        {
            LogError("authInfo->sharedAccessKey input parameter cannot be NULL");
            result = NULL;
        }
        else
        {
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_003: [ IoTHubDeviceMethod_Create shall allocate memory for a new IOTHUB_SERVICE_CLIENT_DEVICE_METHOD_HANDLE instance ]*/
            result = malloc(sizeof(IOTHUB_DEVICE_CONFIGURATION));
            if (result == NULL)
            {
                /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_002: [ If the allocation failed, IoTHubDeviceConfiguration_Create shall return NULL ]*/
                LogError("Malloc failed for IOTHUB_DEVICE_CONFIGURATION");
            }
            else
            {
                memset(result, 0, sizeof(IOTHUB_DEVICE_CONFIGURATION));

                /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_005: [ If the allocation successful, IoTHubDeviceConfiguration_Create shall create a IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE from the given IOTHUB_SERVICE_CLIENT_AUTH_HANDLE and return with it ]*/
                /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_006: [ IoTHubDeviceConfiguration_Create shall allocate memory and copy hostName to result->hostName by calling mallocAndStrcpy_s. ]*/
                if (mallocAndStrcpy_s(&result->hostname, serviceClientAuth->hostname) != 0)
                {
                    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_007: [ If the mallocAndStrcpy_s fails, IoTHubDeviceConfiguration_Create shall do clean up and return NULL. ]*/
                    LogError("mallocAndStrcpy_s failed for hostName");
                    free_deviceConfiguration_handle(result);
                    result = NULL;
                }
                /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_012: [ IoTHubDeviceConfiguration_Create shall allocate memory and copy sharedAccessKey to result->sharedAccessKey by calling mallocAndStrcpy_s. ]*/
                else if (mallocAndStrcpy_s(&result->sharedAccessKey, serviceClientAuth->sharedAccessKey) != 0)
                {
                    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_013: [ If the mallocAndStrcpy_s fails, IoTHubDeviceConfiguration_Create shall do clean up and return NULL. ]*/
                    LogError("mallocAndStrcpy_s failed for sharedAccessKey");
                    free_deviceConfiguration_handle(result);
                    result = NULL;
                }
                /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_014: [ IoTHubDeviceConfiguration_Create shall allocate memory and copy keyName to result->keyName by calling mallocAndStrcpy_s. ]*/
                else if (mallocAndStrcpy_s(&result->keyName, serviceClientAuth->keyName) != 0)
                {
                    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_015: [ If the mallocAndStrcpy_s fails, IoTHubDeviceConfiguration_Create shall do clean up and return NULL. ]*/
                    LogError("mallocAndStrcpy_s failed for keyName");
                    free_deviceConfiguration_handle(result);
                    result = NULL;
                }
            }
        }
    }

    return result;
}

void IoTHubDeviceConfiguration_Destroy(IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE serviceClientDeviceConfigurationHandle)
{
    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_016: [ If the serviceClientDeviceConfigurationHandle input parameter is NULL IoTHubDeviceConfiguration_Destroy shall return ]*/
    if (serviceClientDeviceConfigurationHandle != NULL)
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_17: [ If the serviceClientDeviceConfigurationHandle input parameter is not NULL IoTHubDeviceConfiguration_Destroy shall free the memory of it and return ]*/
        free_deviceConfiguration_handle((IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION*)serviceClientDeviceConfigurationHandle);
    }
}

IOTHUB_DEVICE_CONFIGURATION_RESULT IoTHubDeviceConfiguration_GetConfigurations(IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE serviceClientDeviceConfigurationHandle, const int maxConfigurationsCount, IOTHUB_DEVICE_CONFIGURATIONS_RESULT* configurations)
{
    (void)serviceClientDeviceConfigurationHandle;
    (void)maxConfigurationsCount;
    (void)configurations;

    return IOTHUB_DEVICE_CONFIGURATION_OK;
}

IOTHUB_DEVICE_CONFIGURATION_RESULT IoTHubDeviceConfiguration_GetConfiguration(IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE serviceClientDeviceConfigurationHandle, const char* configurationId, IOTHUB_DEVICE_CONFIGURATION* configuration)
{
    IOTHUB_DEVICE_CONFIGURATION_RESULT result = IOTHUB_DEVICE_CONFIGURATION_OK;

    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_18: [ IoTHubDeviceConfiguration_GetConfiguration shall verify the input parameters and if any of them are NULL then return IOTHUB_DEVICE_CONFIGURATION_INVALID_ARG ]*/
    if ((serviceClientDeviceConfigurationHandle == NULL) || (configurationId == NULL))
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_DEVICE_CONFIGURATION_INVALID_ARG;
    }

    (void)configuration;

    return result;
}

IOTHUB_DEVICE_CONFIGURATION_RESULT IoTHubDeviceConfiguration_AddConfiguration(IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE serviceClientDeviceConfigurationHandle, const IOTHUB_DEVICE_CONFIGURATION_CREATE* configurationCreate, IOTHUB_DEVICE_CONFIGURATION* configuration)
{
    (void)serviceClientDeviceConfigurationHandle;
    (void)configurationCreate;
    (void)configuration;

    return IOTHUB_DEVICE_CONFIGURATION_OK;
}

IOTHUB_DEVICE_CONFIGURATION_RESULT IoTHubDeviceConfiguration_UpdateConfiguration(IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE serviceClientDeviceConfigurationHandle, const IOTHUB_DEVICE_CONFIGURATION_UPDATE* configurationUpdate, IOTHUB_DEVICE_CONFIGURATION* configuration)
{
    (void)serviceClientDeviceConfigurationHandle;
    (void)configurationUpdate;
    (void)configuration;

    return IOTHUB_DEVICE_CONFIGURATION_OK;
}

IOTHUB_DEVICE_CONFIGURATION_RESULT IoTHubDeviceConfiguration_DeleteConfiguration(IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE serviceClientDeviceConfigurationHandle, const char* configurationId)
{
    (void)serviceClientDeviceConfigurationHandle;
    (void)configurationId;

    return IOTHUB_DEVICE_CONFIGURATION_OK;
}
