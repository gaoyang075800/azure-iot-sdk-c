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

IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE IoTHubDeviceConfiguration_Create(IOTHUB_SERVICE_CLIENT_AUTH_HANDLE serviceClientHandle)
{
    /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_001: [ If the serviceClientHandle input parameter is NULL IoTHubDeviceConfiguration_Create shall return NULL ]*/
    (void)serviceClientHandle;
    IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE result;

    result = malloc(sizeof(IOTHUB_DEVICE_CONFIGURATION));
    if (result == NULL)
    {
        /*Codes_SRS_IOTHUBDEVICECONFIGURATION_01_002: [ If the allocation failed, IoTHubDeviceConfiguration_Create shall return NULL ]*/
        LogError("Malloc failed for IOTHUB_DEVICE_CONFIGURATION");
    }

    memset(result, 0, sizeof(IOTHUB_DEVICE_CONFIGURATION));

    return result;
}

void IoTHubDeviceConfiguration_Destroy(IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE serviceClientDeviceConfigurationHandle)
{
    (void)serviceClientDeviceConfigurationHandle;
}

IOTHUB_DEVICE_CONFIGURATION_RESULT IoTHubDeviceConfiguration_GetConfiguration(IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE serviceClientDeviceConfigurationHandle, const char* configurationId, IOTHUB_DEVICE_CONFIGURATION* configuration)
{
    (void)serviceClientDeviceConfigurationHandle;
    (void)configurationId;
    (void)configuration;

    return IOTHUB_DEVICE_CONFIGURATION_OK;
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
