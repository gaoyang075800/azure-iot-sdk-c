// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstring>
#else
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#endif

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

void* my_gballoc_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t l = strlen(source);
    *destination = (char*)my_gballoc_malloc(l + 1);
    strcpy(*destination, source);
    return 0;
}

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_charptr.h"
#include "umocktypes_bool.h"
#include "umocktypes_stdint.h"


#define ENABLE_MOCKS

#include "azure_c_shared_utility/gballoc.h"

#include "internal/iothub_client_private.h"
#include "iothub_client_options.h"

#include "azure_c_shared_utility/platform.h"

#include "azure_c_shared_utility/httpheaders.h"
#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/httpapiexsas.h"
#include "azure_c_shared_utility/uniqueid.h"

#undef ENABLE_MOCKS

#include "azure_c_shared_utility/strings.h"

TEST_DEFINE_ENUM_TYPE(HTTPAPI_RESULT, HTTPAPI_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTPAPI_RESULT, HTTPAPI_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(HTTPAPIEX_RESULT, HTTPAPIEX_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTPAPIEX_RESULT, HTTPAPIEX_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE_VALUES);

static unsigned char* TEST_UNSIGNED_CHAR_PTR = (unsigned char*)"TestString";

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    (void)error_code;
    ASSERT_FAIL("umock_c reported error");
}

STRING_HANDLE my_STRING_construct(const char* psz)
{
    (void)psz;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

void my_STRING_delete(STRING_HANDLE handle)
{
    my_gballoc_free(handle);
}

HTTP_HEADERS_HANDLE my_HTTPHeaders_Alloc(void)
{
    return (HTTP_HEADERS_HANDLE)my_gballoc_malloc(1);
}

void my_HTTPHeaders_Free(HTTP_HEADERS_HANDLE handle)
{
    my_gballoc_free(handle);
}

BUFFER_HANDLE my_BUFFER_new(void)
{
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

BUFFER_HANDLE my_BUFFER_create(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

void my_BUFFER_delete(BUFFER_HANDLE handle)
{
    my_gballoc_free(handle);
}

HTTPAPIEX_HANDLE my_HTTPAPIEX_Create(const char* hostName)
{
    (void)hostName;
    return (HTTPAPIEX_HANDLE)my_gballoc_malloc(1);
}

void my_HTTPAPIEX_Destroy(HTTPAPIEX_HANDLE handle)
{
    my_gballoc_free(handle);
}

HTTPAPIEX_SAS_HANDLE my_HTTPAPIEX_SAS_Create(STRING_HANDLE key, STRING_HANDLE uriResource, STRING_HANDLE keyName)
{
    (void)key;
    (void)uriResource;
    (void)keyName;
    return (HTTPAPIEX_SAS_HANDLE)my_gballoc_malloc(1);
}

void my_HTTPAPIEX_SAS_Destroy(HTTPAPIEX_SAS_HANDLE handle)
{
    my_gballoc_free(handle);
}
#include "iothub_deviceconfiguration.h"

BEGIN_TEST_SUITE(iothub_deviceconfiguration_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    (void)umock_c_init(on_umock_c_error);

    int result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);


}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    umock_c_negative_tests_deinit();
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_001: [ If the serviceClientHandle input parameter is NULL IoTHubDeviceConfiguration_Create shall return NULL ]*/
TEST_FUNCTION(IoTHubDeviceConfiguration_Create_return_null_if_input_parameter_serviceClientHandle_is_NULL)
{
    IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE result = IoTHubDeviceConfiguration_Create(NULL);

    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(iothub_deviceconfiguration_ut)