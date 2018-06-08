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

static const char* CONFIGURATION_JSON_KEY_CONFIGURATION_ID = "id";
static const char* CONFIGURATION_JSON_KEY_SCHEMA_VERSION = "schemaVersion";
static const char* CONFIGURATION_JSON_KEY_DEVICE_CONTENT = "content.deviceContent";
static const char* CONFIGURATION_JSON_KEY_MODULES_CONTENT = "content.modulesContent";
static const char* CONFIGURATION_JSON_KEY_CONTENT_TYPE = "contentType";
static const char* CONFIGURATION_JSON_KEY_TARGET_CONDITION = "targetCondition";
static const char* CONFIGURATION_JSON_KEY_CREATED_TIME = "createdTimeUtc";
static const char* CONFIGURATION_JSON_KEY_LAST_UPDATED_TIME = "lastUpdatedTimeUtc";
static const char* CONFIGURATION_JSON_KEY_PRIORITY = "priority";
static const char* CONFIGURATION_JSON_KEY_SYSTEM_METRICS_RESULTS = "systemMetrics.results";
static const char* CONFIGURATION_JSON_KEY_SYSTEM_METRICS_QUERIES = "systemMetrics.queries";
static const char* CONFIGURATION_JSON_KEY_CUSTOM_METRICS_RESULTS = "metrics.results";
static const char* CONFIGURATION_JSON_KEY_CUSTOM_METRICS_QUERIES = "metrics.queries";
static const char* CONFIGURATION_JSON_KEY_ETAG = "etag";
static const char* CONFIGURATION_CONTENT_PAYLOAD = "{\"deviceContent\": \"%s\", \"modulesContent\": \"%s\"}";

static const char* const URL_API_VERSION = "api-version=2018-03-01-preview";

static const char* const RELATIVE_PATH_FMT_DEVICECONFIGURATION = "/configurations/%s?%s";
static const char* const RELATIVE_PATH_FMT_DEVICECONFIGURATIONS = "/configurations/?top=%s&%s";
static const char* const RELATIVE_PATH_FMT_DEVICECONFIGURATION_TESTQUERIES = "/configurationstestQueries?%s";

static const char* const CONFIGURATION_DEFAULT_CONTENT_TYPE = "assignment";

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
    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_020: [ IoTHubDeviceConfiguration_GetConfiguration shall add the following headers to the created HTTP GET request: authorization=sasToken,Request-Id=<generatedGuid>,Accept=application/json,Content-Type=application/json,charset=utf-8 ]*/
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
    else if ((iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_ADD) || (iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_UPDATE) || (iotHubDeviceConfigurationRequestMode == IOTHUB_DEVICECONFIGURATION_REQUEST_DELETE))
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

static IOTHUB_DEVICE_CONFIGURATION_RESULT sendHttpRequestDeviceConfiguration(IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE serviceClientDeviceConfigurationHandle, IOTHUB_DEVICECONFIGURATION_REQUEST_MODE iotHubDeviceConfigurationRequestMode, const char* configurationId, BUFFER_HANDLE configurationJson, BUFFER_HANDLE responseBuffer)
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
    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_020: [ IoTHubDeviceConfiguration_GetConfiguration shall add the following headers to the created HTTP GET request: authorization=sasToken,Request-Id=<generatedGuid>,Accept=application/json,Content-Type=application/json,charset=utf-8 ]*/
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
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_019: [ IoTHubDeviceConfiguration_GetConfiguration shall create HTTP GET request URL using the given configurationId using the following format: url/configurations/[configurationId] ]*/
            if ((relativePath = createRelativePath(iotHubDeviceConfigurationRequestMode, configurationId)) == NULL)
            {
                /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_024: [ If any of the call fails during the HTTP creation IoTHubDeviceConfiguration_GetConfiguration shall fail and return NULL ]*/
                LogError("Failure creating relative path");
                result = IOTHUB_DEVICE_CONFIGURATION_ERROR;
            }
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ IoTHubDeviceConfiguration_GetConfiguration shall execute the HTTP GET request by calling HTTPAPIEX_ExecuteRequest ]*/
            else if (HTTPAPIEX_SAS_ExecuteRequest(httpExApiSasHandle, httpExApiHandle, httpApiRequestType, STRING_c_str(relativePath), httpHeader, configurationJson, &statusCode, NULL, responseBuffer) != HTTPAPIEX_OK)
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

static BUFFER_HANDLE createConfigurationContentPayloadJson(IOTHUB_DEVICE_CONFIGURATION_CONTENT configurationContent)
{
    STRING_HANDLE stringHandle;
    const char* stringHandle_c_str;
    BUFFER_HANDLE result;

    if ((stringHandle = STRING_construct_sprintf(CONFIGURATION_CONTENT_PAYLOAD, configurationContent.deviceContent, configurationContent.modulesContent)) == NULL)
    {
        LogError("STRING_construct_sprintf failed");
        result = NULL;
    }
    else if ((stringHandle_c_str = STRING_c_str(stringHandle)) == NULL)
    {
        LogError("STRING_c_str failed");
        STRING_delete(stringHandle);
        result = NULL;
    }
    else
    {
        result = BUFFER_create((const unsigned char*)stringHandle_c_str, strlen(stringHandle_c_str));
        STRING_delete(stringHandle);
    }
    return result;
}

static IOTHUB_DEVICE_CONFIGURATION_RESULT parseDeviceConfigurationMetricsJsonObject(JSON_Object* systemMetricsResults, JSON_Object* systemMetricsQueries, IOTHUB_DEVICE_CONFIGURATION_METRICS_RESULT* results, IOTHUB_DEVICE_CONFIGURATION_METRICS_DEFINITION* queries)
{
    IOTHUB_DEVICE_CONFIGURATION_RESULT result = IOTHUB_DEVICE_CONFIGURATION_ERROR;
    /*size_t count;
    size_t i;
    const char* key;
    const char* value;
    JSON_Object* resultsObject;
    JSON_Object* queriesObject;*/
    
    (void)systemMetricsResults;
    (void)systemMetricsQueries;
    (void)results;
    (void)queries;

    result = IOTHUB_DEVICE_CONFIGURATION_OK;

    //TODO: Parse results and queries
    /*count = json_object_get_count(root_object);
    
    if (count > 0) 
    {
        for (i = 0; i < count; i++) 
        {
            key = json_object_get_name(object, i);
            if (key == NULL) 
            {
               break;
            }
        
            written = json_serialize_string(key, buf);
        if (written < 0) {
            return -1;
        }
        if (buf != NULL) {
            buf += written;
        }
        written_total += written;
        APPEND_STRING(":");
        if (is_pretty) {
            APPEND_STRING(" ");
        }
        temp_value = json_object_get_value(object, key);
        written = json_serialize_to_buffer_r(temp_value, buf, level + 1, is_pretty, num_buf);
        if (written < 0) {
            return -1;
        }*/

    return result;
}

static IOTHUB_DEVICE_CONFIGURATION_RESULT parseDeviceConfigurationJsonObject(JSON_Object* root_object, IOTHUB_DEVICE_CONFIGURATION* configuration)
{
    IOTHUB_DEVICE_CONFIGURATION_RESULT result = IOTHUB_DEVICE_CONFIGURATION_ERROR;

    const char* configurationId = json_object_get_string(root_object, CONFIGURATION_JSON_KEY_CONFIGURATION_ID);
    const char* schemaVersion = json_object_get_string(root_object, CONFIGURATION_JSON_KEY_SCHEMA_VERSION);
    const char* deviceContent = json_object_dotget_string(root_object, CONFIGURATION_JSON_KEY_DEVICE_CONTENT);
    const char* modulesContent = json_object_dotget_string(root_object, CONFIGURATION_JSON_KEY_MODULES_CONTENT);
    const char* contentType = json_object_get_string(root_object, CONFIGURATION_JSON_KEY_CONTENT_TYPE);
    const char* targetCondition = json_object_get_string(root_object, CONFIGURATION_JSON_KEY_TARGET_CONDITION);
    const char* createdTime = json_object_get_string(root_object, CONFIGURATION_JSON_KEY_CREATED_TIME);
    const char* lastUpdatedTime = json_object_get_string(root_object, CONFIGURATION_JSON_KEY_LAST_UPDATED_TIME);
    const char* priority = json_object_get_string(root_object, CONFIGURATION_JSON_KEY_PRIORITY);
    const char* eTag = json_object_get_string(root_object, CONFIGURATION_JSON_KEY_ETAG);

    JSON_Object* systemMetricsResults = json_object_dotget_object(root_object, CONFIGURATION_JSON_KEY_SYSTEM_METRICS_RESULTS);
    JSON_Object* systemMetricsQueries = json_object_dotget_object(root_object, CONFIGURATION_JSON_KEY_SYSTEM_METRICS_QUERIES);
    JSON_Object* customMetricsResults = json_object_dotget_object(root_object, CONFIGURATION_JSON_KEY_CUSTOM_METRICS_RESULTS);
    JSON_Object* customMetricsQueries = json_object_dotget_object(root_object, CONFIGURATION_JSON_KEY_CUSTOM_METRICS_QUERIES);

    if ((configurationId != NULL) && (mallocAndStrcpy_s((char**)&(configuration->configurationId), configurationId) != 0))
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ If the JSON parsing failed, IoTHubDeviceConfiguration_AddConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        LogError("mallocAndStrcpy_s failed for configurationId");
        result = IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR;
    }
    else if ((schemaVersion != NULL) && (mallocAndStrcpy_s((char**)&(configuration->schemaVersion), schemaVersion) != 0))
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ If the JSON parsing failed, IoTHubDeviceConfiguration_AddConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        LogError("mallocAndStrcpy_s failed for schemaVersion");
        result = IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR;
    }
    else if ((deviceContent != NULL) && (mallocAndStrcpy_s((char**)&configuration->content.deviceContent, deviceContent) != 0))
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ If the JSON parsing failed, IoTHubDeviceConfiguration_AddConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        LogError("mallocAndStrcpy_s failed for content.deviceContent");
        result = IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR;
    }
    else if ((modulesContent != NULL) && (mallocAndStrcpy_s((char**)&configuration->content.modulesContent, modulesContent) != 0))
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ If the JSON parsing failed, IoTHubDeviceConfiguration_AddConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        LogError("mallocAndStrcpy_s failed for content.modulesContent");
        result = IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR;
    }
    else if ((eTag != NULL) && (mallocAndStrcpy_s((char**)&configuration->eTag, eTag) != 0))
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ If the JSON parsing failed, IoTHubDeviceConfiguration_AddConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        LogError("mallocAndStrcpy_s failed for eTag");
        result = IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR;
    }
    else if ((contentType != NULL) && (mallocAndStrcpy_s((char**)&configuration->contentType, contentType) != 0))
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ If the JSON parsing failed, IoTHubDeviceConfiguration_AddConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        LogError("mallocAndStrcpy_s failed for contentType");
        result = IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR;
    }
    else if ((targetCondition != NULL) && (mallocAndStrcpy_s((char**)&configuration->targetCondition, targetCondition) != 0))
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ If the JSON parsing failed, IoTHubDeviceConfiguration_AddConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        LogError("mallocAndStrcpy_s failed for targetCondition");
        result = IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR;
    }
    else if ((createdTime != NULL) && (mallocAndStrcpy_s((char**)&configuration->createdTimeUtc, createdTime) != 0))
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ If the JSON parsing failed, IoTHubDeviceConfiguration_AddConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        LogError("mallocAndStrcpy_s failed for createdTimeUtc");
        result = IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR;
    }
    else if ((lastUpdatedTime != NULL) && (mallocAndStrcpy_s((char**)&configuration->lastUpdatedTimeUtc, lastUpdatedTime) != 0))
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ If the JSON parsing failed, IoTHubDeviceConfiguration_AddConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        LogError("mallocAndStrcpy_s failed for lastUpdatedTimeUtc");
        result = IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR;
    }
    else
    {
        if (priority != NULL)
        {
            configuration->priority = atoi(priority);
        }

        //TODO: error handling
        result = parseDeviceConfigurationMetricsJsonObject(systemMetricsResults, systemMetricsQueries, &(configuration->systemMetricsResult), &(configuration->systemMetricsDefinition));

        //TODO: error handling
        result = parseDeviceConfigurationMetricsJsonObject(customMetricsResults, customMetricsQueries, &(configuration->metricResult), &(configuration->metricsDefinition));
    }

    return result;
}

void IoTHubDeviceConfiguration_FreeConfigurationMembers(IOTHUB_DEVICE_CONFIGURATION* configuration)
{
    free((char *)configuration->configurationId);
    free((char *)configuration->schemaVersion);
    free((char *)configuration->targetCondition);
    free((char *)configuration->eTag);
    free((char *)configuration->contentType);
    free((char *)configuration->createdTimeUtc);
    free((char *)configuration->lastUpdatedTimeUtc);
    memset(configuration, 0, sizeof(*configuration));
}

static IOTHUB_DEVICE_CONFIGURATION_RESULT parseDeviceConfigurationJson(BUFFER_HANDLE jsonBuffer, IOTHUB_DEVICE_CONFIGURATION* configuration)
{
    IOTHUB_DEVICE_CONFIGURATION_RESULT result = IOTHUB_DEVICE_CONFIGURATION_ERROR;

    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_024: [ If the configuration out parameter is not NULL IoTHubDeviceConfiguration_AddConfiguration shall save the received configuration to the out parameter and return IOTHUB_DEVICE_CONFIGURATION_OK ] */
    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_033: [ IoTHubDeviceConfiguration_GetConfiguration shall verify the received HTTP status code and if it is less or equal than 300 then try to parse the response JSON to configuration for configuration properties ] */
    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_034: [ If any of the property field above missing from the JSON the property value will not be populated ] */
    if (jsonBuffer == NULL)
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ If the JSON parsing failed, IoTHubDeviceConfiguration_AddConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        LogError("jsonBuffer cannot be NULL");
        result = IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR;
    }
    else if (configuration == NULL)
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ If the JSON parsing failed, IoTHubDeviceConfiguration_AddConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
        LogError("configuration cannot be NULL");
        result = IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR;
    }
    else
    {
        const char* bufferStr = NULL;
        JSON_Value* root_value = NULL;
        JSON_Object* root_object = NULL;
        JSON_Status jsonStatus;

        if ((bufferStr = (const char*)BUFFER_u_char(jsonBuffer)) == NULL)
        {
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ If the JSON parsing failed, IoTHubDeviceConfiguration_AddConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
            LogError("BUFFER_u_char failed");
            result = IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR;
        }
        else if ((root_value = json_parse_string(bufferStr)) == NULL)
        {
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ If the JSON parsing failed, IoTHubDeviceConfiguration_AddConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
            LogError("json_parse_string failed");
            result = IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR;
        }
        else if ((root_object = json_value_get_object(root_value)) == NULL)
        {
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ If the JSON parsing failed, IoTHubDeviceConfiguration_AddConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
            LogError("json_value_get_object failed");
            result = IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR;
        }
        else
        {
            result = parseDeviceConfigurationJsonObject(root_object, configuration);
        }

        if ((jsonStatus = json_object_clear(root_object)) != JSONSuccess)
        {
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ If the JSON parsing failed, IoTHubDeviceConfiguration_AddConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
            LogError("json_object_clear failed");
            result = IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR;
        }

        json_value_free(root_value);

        if (result != IOTHUB_DEVICE_CONFIGURATION_OK)
        {
            IoTHubDeviceConfiguration_FreeConfigurationMembers(configuration);
        }
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

IOTHUB_DEVICE_CONFIGURATION_RESULT IoTHubDeviceConfiguration_GetConfigurations(IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE serviceClientDeviceConfigurationHandle, const int maxConfigurationsCount, SINGLYLINKEDLIST_HANDLE configurations)
{
    (void)serviceClientDeviceConfigurationHandle;
    (void)maxConfigurationsCount;
    (void)configurations;

    return IOTHUB_DEVICE_CONFIGURATION_OK;
}

static void initializeDeviceConfigurationMembers(IOTHUB_DEVICE_CONFIGURATION * configuration)
{
    if (NULL != configuration)
    {
        memset(configuration, 0, sizeof(IOTHUB_DEVICE_CONFIGURATION));
    }
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
    else
    {
        configuration = malloc(sizeof(IOTHUB_DEVICE_CONFIGURATION));
        initializeDeviceConfigurationMembers(configuration);

        BUFFER_HANDLE responseBuffer;

        if ((responseBuffer = BUFFER_new()) == NULL)
        {
            LogError("BUFFER_new failed for responseBuffer");
            result = IOTHUB_DEVICE_CONFIGURATION_ERROR;
        }
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_026: [ IoTHubDeviceConfiguration_GetConfiguration shall create HTTP GET request URL using the given configurationId using the following format: url/devices/[configurationId]?[apiVersion]  ] */
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_027: [ IoTHubDeviceConfiguration_GetConfiguration shall add the following headers to the created HTTP GET request: authorization=sasToken,Request-Id=<generatedGuid>,Accept=application/json,Content-Type=application/json,charset=utf-8 ] */
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_028: [ IoTHubDeviceConfiguration_GetConfiguration shall create an HTTPAPIEX_SAS_HANDLE handle by calling HTTPAPIEX_SAS_Create ] */
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_029: [ IoTHubDeviceConfiguration_GetConfiguration shall create an HTTPAPIEX_HANDLE handle by calling HTTPAPIEX_Create ] */
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_030: [ IoTHubDeviceConfiguration_GetConfiguration shall execute the HTTP GET request by calling HTTPAPIEX_ExecuteRequest ] */
        else if ((result = sendHttpRequestDeviceConfiguration(serviceClientDeviceConfigurationHandle, IOTHUB_DEVICECONFIGURATION_REQUEST_GET, configurationId, NULL, responseBuffer)) == IOTHUB_DEVICE_CONFIGURATION_ERROR)
        {
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_031: [ If any of the HTTPAPI call fails IoTHubDeviceConfiguration_GetConfiguration shall fail and return IOTHUB_DEVICE_CONFIGURATION_ERROR ] */
            LogError("Failure sending HTTP request for get device configuration");
        }
        else if (result == IOTHUB_DEVICE_CONFIGURATION_OK)
        {
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_033: [ IoTHubDeviceConfiguration_GetConfiguration shall verify the received HTTP status code and if it is less or equal than 300 then try to parse the response JSON to configuration for the configuration properties] */
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_034: [ If any of the property field above missing from the JSON the property value will not be populated ] */
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_035: [ If the JSON parsing failed, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR ] */
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_036: [ If the received JSON is empty, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_DEVICE_NOT_EXIST ] */
            /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_037: [ If the configuration out parameter if not NULL IoTHubDeviceConfiguration_GetConfiguration shall save the received configuration to the out parameter and return IOTHUB_DEVICE_CONFIGURATION_OK ] */
            if ((result = parseDeviceConfigurationJson(responseBuffer, configuration)) == IOTHUB_DEVICE_CONFIGURATION_OK)
            {
                /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_036: [ If the received JSON is empty, IoTHubDeviceConfiguration_GetConfiguration shall return IOTHUB_DEVICE_CONFIGURATION_DEVICE_NOT_EXIST ] */
                if (configuration->configurationId == NULL)
                {
                    IoTHubDeviceConfiguration_FreeConfigurationMembers(configuration);
                    result = IOTHUB_DEVICE_CONFIGURATION_CONFIGURATION_NOT_EXIST;
                }
            }
        }

        BUFFER_delete(responseBuffer);
    }

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
	IOTHUB_DEVICE_CONFIGURATION_RESULT result;

	/*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_052: [ IoTHubDeviceConfiguration_DeleteConfiguration shall verify the input parameters and if any of them are NULL then return IOTHUB_DEVICE_CONFIGURATION_INVALID_ARG ]*/
	if ((serviceClientDeviceConfigurationHandle == NULL) || (configurationId == NULL))
	{
		LogError("Input parameter cannot be NULL");
		result = IOTHUB_DEVICE_CONFIGURATION_INVALID_ARG;
	}
	else
	{
		/*SRS_IOTHUBDEVICECONFIGURATION_01_053: [ IoTHubDeviceConfiguration_DeleteConfiguration shall create HTTP DELETE request URL using the given configurationId using the following format : url/configurations/[configurationId]?api-version  ] */
		/*SRS_IOTHUBDEVICECONFIGURATION_01_054: [ IoTHubDeviceConfiguration_DeleteConfiguration shall add the following headers to the created HTTP GET request : authorization=sasToken,Request-Id=<generatedGuid>,Accept=application/json,Content-Type=application/json,charset=utf-8 ] */
		/*SRS_IOTHUBDEVICECONFIGURATION_01_055: [ IoTHubDeviceConfiguration_DeleteConfiguration shall create an HTTPAPIEX_SAS_HANDLE handle by calling HTTPAPIEX_SAS_Create ] */
		/*SRS_IOTHUBDEVICECONFIGURATION_01_056: [ IoTHubDeviceConfiguration_DeleteConfiguration shall create an HTTPAPIEX_HANDLE handle by calling HTTPAPIEX_Create ] */
		/*SRS_IOTHUBDEVICECONFIGURATION_01_057: [ IoTHubDeviceConfiguration_DeleteConfiguration shall execute the HTTP DELETE request by calling HTTPAPIEX_ExecuteRequest ] */
		/*SRS_IOTHUBDEVICECONFIGURATION_01_058: [ IoTHubDeviceConfiguration_DeleteConfiguration shall verify the received HTTP status code and if it is greater than 300 then return IOTHUB_DEVICE_CONFIGURATION_HTTP_STATUS_ERROR ] */
		/*SRS_IOTHUBDEVICECONFIGURATION_01_059: [ IoTHubDeviceConfiguration_DeleteConfiguration shall verify the received HTTP status code and if it is less or equal than 300 then return IOTHUB_DEVICE_CONFIGURATION_OK ] */
		result = sendHttpRequestDeviceConfiguration(serviceClientDeviceConfigurationHandle, IOTHUB_DEVICECONFIGURATION_REQUEST_DELETE, configurationId, NULL, NULL);
	}

	return result;
}