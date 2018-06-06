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
    IOTHUB_DEVICECONFIGURATION_REQUEST_GET,             \
    IOTHUB_DEVICECONFIGURATION_REQUEST_GET_SINGLE,      \
    IOTHUB_DEVICECONFIGURATION_REQUEST_ADD,             \
    IOTHUB_DEVICECONFIGURATION_REQUEST_UPDATE,          \
    IOTHUB_DEVICECONFIGURATION_REQUEST_DELETE          

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
static const char* const RELATIVE_PATH_FMT_APPLY_DEVICECONFIGURATION = "/devices/%s/applyConfigurationContent?";
static const char* const RELATIVE_PATH_FMT_DEVICECONFIGURATION_PAYLOAD = "{\"methodName\":\"%s\",\"timeout\":%d,\"payload\":%s}";

typedef struct IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_TAG
{
    char* hostname;
    char* sharedAccessKey;
    char* keyName;
} IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION;

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
    if(serviceClientHandle == NULL)
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
