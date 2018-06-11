// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/map.h"
#include "iothub_service_client_auth.h"

#include "iothub_deviceconfiguration.h"

static const char* connectionString = "[Connection String]";
static const char* configurationId = "[Configuration Id]";
static const char* targetCondition = "tags.UniqueTag='configurationapplyedgeagentreportinge2etestcita5b4e2b7f6464fe9988feea7d887584a' and tags.Environment='test'";
static const char* updatedTargetCondition = "tags.Environment='test'";
static const char* deviceContent = "{\"properties.desired.settings1\": {\"c\": 3, \"d\" : 4}, \"properties.desired.settings2\" : \"xyz\"}";
static const char* modulesContent = "{\"sunny\": {\"properties.desired\": {\"temperature\": 69,\"humidity\": 30}},\"goolily\": {\"properties.desired\": {\"elevation\": 45,\"orientation\": \"NE\"}},\"$edgeAgent\": {\"properties.desired\": {\"schemaVersion\": \"1.0\",\"runtime\": {\"type\": \"docker\",\"settings\": {\"minDockerVersion\": \"1.5\",\"loggingOptions\": \"\"}},\"systemModules\": {\"edgeAgent\": {\"type\": \"docker\",\"settings\": {\"image\": \"edgeAgent\",\"createOptions\": \"\"},\"configuration\": {\"id\": \"configurationapplyedgeagentreportinge2etestcit-config-a9ed4811-1b57-48bf-8af2-02319a38de01\"}},\"edgeHub\": {\"type\": \"docker\",\"status\": \"running\",\"restartPolicy\": \"always\",\"settings\": {\"image\": \"edgeHub\",\"createOptions\": \"\"},\"configuration\": {\"id\": \"configurationapplyedgeagentreportinge2etestcit-config-a9ed4811-1b57-48bf-8af2-02319a38de01\"}}},\"modules\": {\"sunny\": {\"version\": \"1.0\",\"type\": \"docker\",\"status\": \"running\",\"restartPolicy\": \"on-failure\",\"settings\": {\"image\": \"mongo\",\"createOptions\": \"\"},\"configuration\": {\"id\": \"configurationapplyedgeagentreportinge2etestcit-config-a9ed4811-1b57-48bf-8af2-02319a38de01\"}},\"goolily\": {\"version\": \"1.0\",\"type\": \"docker\",\"status\": \"running\",\"restartPolicy\": \"on-failure\",\"settings\": {\"image\": \"asa\",\"createOptions\": \"\"},\"configuration\": {\"id\": \"configurationapplyedgeagentreportinge2etestcit-config-a9ed4811-1b57-48bf-8af2-02319a38de01\"}}}}},\"$edgeHub\": {\"properties.desired\": {\"schemaVersion\": \"1.0\",\"routes\": {\"route1\": \"from * INTO $upstream\"},\"storeAndForwardConfiguration\": {\"timeToLiveSecs\": 20}}}}";


int main(void)
{
    (void)platform_init();

    IOTHUB_DEVICE_CONFIGURATION_RESULT result;
    IOTHUB_DEVICE_CONFIGURATION deviceConfigurationInfo;

    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(connectionString);
    if (iotHubServiceClientHandle == NULL)
    {
        (void)printf("IoTHubServiceClientAuth_CreateFromConnectionString failed\n");
    }
    else
    {
        // Add configuration
        IOTHUB_DEVICE_CONFIGURATION_ADD deviceConfigurationAddInfo;
        IOTHUB_DEVICE_CONFIGURATION_CONTENT deviceConfigurationAddInfoContent;
        IOTHUB_DEVICE_CONFIGURATION_LABELS deviceConfigurationAddInfoLabels;
        MAP_HANDLE labels = Map_Create(NULL);

        IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE iotHubDeviceConfigurationHandle = IoTHubDeviceConfiguration_Create(iotHubServiceClientHandle);

        (void)memset(&deviceConfigurationAddInfo, 0, sizeof(IOTHUB_DEVICE_CONFIGURATION_ADD));
        (void)memset(&deviceConfigurationAddInfoContent, 0, sizeof(IOTHUB_DEVICE_CONFIGURATION_CONTENT));
        (void)memset(&deviceConfigurationAddInfoLabels, 0, sizeof(IOTHUB_DEVICE_CONFIGURATION_LABELS));

        deviceConfigurationAddInfoContent.deviceContent = deviceContent;
        deviceConfigurationAddInfoContent.modulesContent = modulesContent;
        
        Map_Add(labels, "label1", "value1");               
        Map_GetInternals(labels, &deviceConfigurationAddInfoLabels.labelName, &deviceConfigurationAddInfoLabels.labelValue, &deviceConfigurationAddInfoLabels.numLabels);
        
        deviceConfigurationAddInfo.configurationId = configurationId;
        deviceConfigurationAddInfo.targetCondition = targetCondition;
        deviceConfigurationAddInfo.content = deviceConfigurationAddInfoContent;
        deviceConfigurationAddInfo.labels = deviceConfigurationAddInfoLabels;
        deviceConfigurationAddInfo.priority = 10;
        deviceConfigurationAddInfo.version = IOTHUB_DEVICE_CONFIGURATION_ADD_VERSION_1;

        result = IoTHubDeviceConfiguration_AddConfiguration(iotHubDeviceConfigurationHandle, &deviceConfigurationAddInfo, &deviceConfigurationInfo);
        if (result == IOTHUB_DEVICE_CONFIGURATION_OK)
        {

        }
        else if (result == IOTHUB_DEVICE_CONFIGURATION_CONFIGURATION_EXIST)
        {

        }
        else if (result == IOTHUB_DEVICE_CONFIGURATION_ERROR)
        {

        }

        //TODO: enable after implementing Add
        //IoTHubDeviceConfiguration_FreeConfigurationMembers(&deviceConfigurationInfo);
        Map_Destroy(labels);

        // Update configuration
        IOTHUB_DEVICE_CONFIGURATION_UPDATE deviceConfigurationUpdateInfo;

        (void)memset(&deviceConfigurationUpdateInfo, 0, sizeof(IOTHUB_DEVICE_CONFIGURATION_UPDATE));
        deviceConfigurationUpdateInfo.configurationId = configurationId;
        deviceConfigurationUpdateInfo.targetCondition = updatedTargetCondition;
        deviceConfigurationUpdateInfo.version = IOTHUB_DEVICE_CONFIGURATION_UPDATE_VERSION_1;

        result = IoTHubDeviceConfiguration_UpdateConfiguration(iotHubDeviceConfigurationHandle, &deviceConfigurationUpdateInfo, &deviceConfigurationInfo);
        if (result == IOTHUB_DEVICE_CONFIGURATION_OK)
        {

        }
        else if (result == IOTHUB_DEVICE_CONFIGURATION_CONFIGURATION_NOT_EXIST)
        {

        }
        else if (result == IOTHUB_DEVICE_CONFIGURATION_ERROR)
        {

        }

        //TODO: Implement after enabling
        //IoTHubDeviceConfiguration_FreeConfigurationMembers(&deviceConfigurationInfo);

        // Get configuration

        (void)memset(&deviceConfigurationInfo, 0, sizeof(IOTHUB_DEVICE_CONFIGURATION));

        result = IoTHubDeviceConfiguration_GetConfiguration(iotHubDeviceConfigurationHandle, deviceConfigurationAddInfo.configurationId, &deviceConfigurationInfo);
        if (result == IOTHUB_DEVICE_CONFIGURATION_OK)
        {

        }
        else if (result == IOTHUB_DEVICE_CONFIGURATION_CONFIGURATION_NOT_EXIST)
        {

        }
        else if (result == IOTHUB_DEVICE_CONFIGURATION_ERROR)
        {

        }

        IoTHubDeviceConfiguration_FreeConfigurationMembers(&deviceConfigurationInfo);

        result = IoTHubDeviceConfiguration_DeleteConfiguration(iotHubDeviceConfigurationHandle, deviceConfigurationAddInfo.configurationId);
        if (result == IOTHUB_DEVICE_CONFIGURATION_OK)
        {

        }
        else if (result == IOTHUB_DEVICE_CONFIGURATION_CONFIGURATION_NOT_EXIST)
        {

        }
        else if (result == IOTHUB_DEVICE_CONFIGURATION_ERROR)
        {

        }
    }
    platform_deinit();
}