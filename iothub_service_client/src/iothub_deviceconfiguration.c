// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iothub_deviceconfiguration.h"

IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE IoTHubDeviceConfiguration_Create(IOTHUB_SERVICE_CLIENT_AUTH_HANDLE serviceClientHandle)
{
    (void)serviceClientHandle;
    
    return NULL;
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
