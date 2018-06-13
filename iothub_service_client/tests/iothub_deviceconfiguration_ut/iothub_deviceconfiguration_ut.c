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
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "parson.h"

MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);
MOCKABLE_FUNCTION(, double, json_object_get_number, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, char*, json_serialize_to_string, const JSON_Value*, value);
MOCKABLE_FUNCTION(, void, json_free_serialized_string, char*, string);
MOCKABLE_FUNCTION(, const char*, json_object_dotget_string, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_string, JSON_Object*, object, const char*, name, const char*, string);
MOCKABLE_FUNCTION(, JSON_Status, json_object_dotset_string, JSON_Object*, object, const char*, name, const char*, string);
MOCKABLE_FUNCTION(, JSON_Value*, json_value_init_object);
MOCKABLE_FUNCTION(, JSON_Array*, json_array_get_array, const JSON_Array*, array, size_t, index);
MOCKABLE_FUNCTION(, JSON_Object*, json_array_get_object, const JSON_Array*, array, size_t, index);
MOCKABLE_FUNCTION(, JSON_Array*, json_value_get_array, const JSON_Value*, value);
MOCKABLE_FUNCTION(, size_t, json_array_get_count, const JSON_Array*, array);
MOCKABLE_FUNCTION(, JSON_Status, json_array_clear, JSON_Array*, array);
MOCKABLE_FUNCTION(, JSON_Status, json_object_clear, JSON_Object*, object);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Status, json_object_dotset_boolean, JSON_Object*, object, const char *, name, int, boolean);
MOCKABLE_FUNCTION(, int, json_object_dotget_boolean, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, JSON_Object*, json_object_dotget_object, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, JSON_Value*, json_object_dotget_value, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_value, JSON_Object *, object, const char *, name, JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_number, JSON_Object *, object, const char *, name, double, number);

MOCKABLE_FUNCTION(, double, json_object_dotget_number, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, size_t, json_object_get_count, const JSON_Object *, object);
MOCKABLE_FUNCTION(, const char *, json_object_get_name, const JSON_Object *, object, size_t, index);
MOCKABLE_FUNCTION(, JSON_Value *, json_object_get_value_at, const JSON_Object *, object, size_t, index);
MOCKABLE_FUNCTION(, int, json_object_has_value, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, const char *, json_value_get_string, const JSON_Value *, value);

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

STRING_HANDLE my_STRING_construct_n(const char* psz, size_t n)
{
    (void)psz;
    n = 0;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

STRING_HANDLE my_STRING_from_byte_array(const unsigned char* psz, size_t n)
{
    (void)psz;
    n = 0;
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

char* my_json_serialize_to_string(const JSON_Value *value)
{
    (void)value;
    char* s = (char*)my_gballoc_malloc(1);
    *s = 0;
    return s;
}

JSON_Value *my_json_parse_string(const char *string)
{
    (void)string;
    return (JSON_Value*)my_gballoc_malloc(1);
}

void my_json_value_free(JSON_Value *value)
{
    my_gballoc_free(value);
}

typedef struct LIST_ITEM_INSTANCE_TAG
{
    const void* item;
    void* next;
} LIST_ITEM_INSTANCE;

typedef struct SINGLYLINKEDLIST_INSTANCE_TAG
{
    LIST_ITEM_INSTANCE* head;
} LIST_INSTANCE;

static SINGLYLINKEDLIST_HANDLE my_list_create(void)
{
    LIST_INSTANCE* result;

    result = (LIST_INSTANCE*)malloc(sizeof(LIST_INSTANCE));
    if (result != NULL)
    {
        result->head = NULL;
    }

    return result;
}

static void my_list_destroy(SINGLYLINKEDLIST_HANDLE list)
{
    if (list != NULL)
    {
        LIST_INSTANCE* list_instance = (LIST_INSTANCE*)list;

        while (list_instance->head != NULL)
        {
            LIST_ITEM_INSTANCE* current_item = list_instance->head;
            list_instance->head = (LIST_ITEM_INSTANCE*)current_item->next;
            free(current_item);
        }

        free(list_instance);
    }
}

static LIST_ITEM_HANDLE my_list_add(SINGLYLINKEDLIST_HANDLE list, const void* item)
{
    LIST_ITEM_INSTANCE* result;

    if ((list == NULL) ||
        (item == NULL))
    {
        result = NULL;
    }
    else
    {
        LIST_INSTANCE* list_instance = (LIST_INSTANCE*)list;
        result = (LIST_ITEM_INSTANCE*)malloc(sizeof(LIST_ITEM_INSTANCE));

        if (result == NULL)
        {
            result = NULL;
        }
        else
        {
            result->next = NULL;
            result->item = item;

            if (list_instance->head == NULL)
            {
                list_instance->head = result;
            }
            else
            {
                LIST_ITEM_INSTANCE* current = list_instance->head;
                while (current->next != NULL)
                {
                    current = (LIST_ITEM_INSTANCE*)current->next;
                }

                current->next = result;
            }
        }
    }

    return result;
}

static LIST_ITEM_HANDLE my_list_get_head_item(SINGLYLINKEDLIST_HANDLE list)
{
    LIST_ITEM_HANDLE result;

    if (list == NULL)
    {
        result = NULL;
    }
    else
    {
        LIST_INSTANCE* list_instance = (LIST_INSTANCE*)list;

        result = list_instance->head;
    }

    return result;
}

static LIST_ITEM_HANDLE my_list_get_next_item(LIST_ITEM_HANDLE item_handle)
{
    LIST_ITEM_HANDLE result;

    if (item_handle == NULL)
    {
        result = NULL;
    }
    else
    {
        result = (LIST_ITEM_HANDLE)((LIST_ITEM_INSTANCE*)item_handle)->next;
    }

    return result;
}

static const void* my_list_item_get_value(LIST_ITEM_HANDLE item_handle)
{
    const void* result;

    if (item_handle == NULL)
    {
        result = NULL;
    }
    else
    {
        result = ((LIST_ITEM_INSTANCE*)item_handle)->item;
    }

    return result;
}

#include "iothub_deviceconfiguration.h"
#include "iothub_service_client_auth.h"

typedef struct IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_TAG
{
    char* hostname;
    char* sharedAccessKey;
    char* keyName;
} IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION;

static IOTHUB_SERVICE_CLIENT_AUTH TEST_IOTHUB_SERVICE_CLIENT_AUTH;
static IOTHUB_SERVICE_CLIENT_AUTH_HANDLE TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE = &TEST_IOTHUB_SERVICE_CLIENT_AUTH;

static IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION TEST_IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION;
static IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE TEST_IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE = &TEST_IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION;

static const char* TEST_STRING_VALUE = "Test string value";
static const char* TEST_CONST_CHAR_PTR = "TestConstChar";
static char* TEST_CHAR_PTR = "TestString";

static const char* TEST_CONFIGURATION_ID = "theid";
static const char* TEST_SCHEMA_VERSION = "theschemaVersion";
static const char* TEST_CONTENT = "thecontent";
static const char* TEST_DEVICE_CONTENT = "thedeviceContent";
static const char* TEST_MODULES_CONTENT = "themodulesContent";
static const char* TEST_CONTENT_TYPE = "thecontentType";
static const char* TEST_TARGET_CONDITION = "thetargetCondition";
static const char* TEST_CREATED_TIME = "thecreatedTimeUtc";
static const char* TEST_LAST_UPDATED_TIME = "thelastUpdatedTimeUtc";
static const char* TEST_PRIORITY = "thepriority";
static const char* TEST_METRICS = "themetrics";
static const char* TEST_RESULTS = "theresults";
static const char* TEST_QUERIES = "thequeries";
static const char* TEST_ETAG = "theetag";
static const char* TEST_CONFIGURATION_DEVICE_CONTENT_PAYLOAD = "{\"properties.desired.%s\":\"%s\",\"timeout\":%d,\"payload\":%s}";

static char* TEST_HOSTNAME = "theHostName";
static char* TEST_IOTHUBNAME = "theIotHubName";
static char* TEST_IOTHUBSUFFIX = "theIotHubSuffix";
static char* TEST_SHAREDACCESSKEY = "theSharedAccessKey";
static char* TEST_SHAREDACCESSKEYNAME = "theSharedAccessKeyName";

static const HTTP_HEADERS_HANDLE TEST_HTTP_HEADERS_HANDLE = (HTTP_HEADERS_HANDLE)0x4545;

static const unsigned int httpStatusCodeOk = 200;
static const unsigned int httpStatusCodeDeleted = 204;
static const unsigned int httpStatusCodeBadRequest = 400;

static const char* TEST_CONFIGURATION_JSON_KEY_CONFIGURATION_ID = "id";
static const char* TEST_CONFIGURATION_JSON_KEY_SCHEMA_VERSION = "schemaVersion";
static const char* TEST_CONFIGURATION_JSON_KEY_CONTENT = "content";
static const char* TEST_CONFIGURATION_JSON_KEY_DEVICE_CONTENT = "deviceContent";
static const char* TEST_CONFIGURATION_JSON_KEY_MODULES_CONTENT = "modulesContent";
static const char* TEST_CONFIGURATION_JSON_KEY_CONTENT_TYPE = "contentType";
static const char* TEST_CONFIGURATION_JSON_KEY_TARGET_CONDITION = "targetCondition";
static const char* TEST_CONFIGURATION_JSON_KEY_CREATED_TIME = "createdTimeUtc";
static const char* TEST_CONFIGURATION_JSON_KEY_LAST_UPDATED_TIME = "lastUpdatedTimeUtc";
static const char* TEST_CONFIGURATION_JSON_KEY_PRIORITY = "priority";
static const char* TEST_CONFIGURATION_JSON_KEY_METRICS = "metrics";
static const char* TEST_CONFIGURATION_JSON_KEY_RESULTS = "results";
static const char* TEST_CONFIGURATION_JSON_KEY_QUERIES = "queries";
static const char* TEST_CONFIGURATION_JSON_KEY_ETAG = "etag";

static const char* TEST_LASTACTIVITYTIME = "0001-01-01T33:33:33";

static const char* TEST_HTTP_HEADER_KEY_AUTHORIZATION = "Authorization";
static const char* TEST_HTTP_HEADER_VAL_AUTHORIZATION = " ";
static const char* TEST_HTTP_HEADER_KEY_REQUEST_ID = "Request-Id";
static const char* TEST_HTTP_HEADER_VAL_REQUEST_ID = "1001";
static const char* TEST_HTTP_HEADER_KEY_USER_AGENT = "User-Agent";
static const char* TEST_HTTP_HEADER_KEY_ACCEPT = "Accept";
static const char* TEST_HTTP_HEADER_VAL_ACCEPT = "application/json";
static const char* TEST_HTTP_HEADER_KEY_CONTENT_TYPE = "Content-Type";
static const char* TEST_HTTP_HEADER_VAL_CONTENT_TYPE = "application/json; charset=utf-8";
static const char* TEST_HTTP_HEADER_KEY_IFMATCH = "If-Match";
static const char* TEST_HTTP_HEADER_VAL_IFMATCH = "*";

static JSON_Value* TEST_JSON_VALUE = (JSON_Value*)0x5050;
static JSON_Object* TEST_JSON_OBJECT = (JSON_Object*)0x5151;
static JSON_Array* TEST_JSON_ARRAY = (JSON_Array*)0x5252;
static JSON_Status TEST_JSON_STATUS = 0;

#ifdef __cplusplus
extern "C"
{
#endif
    int STRING_sprintf(STRING_HANDLE handle, const char* format, ...);
    STRING_HANDLE STRING_construct_sprintf(const char* format, ...);

    int STRING_sprintf(STRING_HANDLE handle, const char* format, ...)
    {
        (void)handle;
        (void)format;
        return 0;
    }

    STRING_HANDLE STRING_construct_sprintf(const char* format, ...)
    {
        (void)format;
        return (STRING_HANDLE)my_gballoc_malloc(1);
    }

    const char* my_STRING_c_str(STRING_HANDLE handle)
    {
        (void)handle;
        return TEST_STRING_VALUE;
    }

#ifdef __cplusplus
}
#endif

BEGIN_TEST_SUITE(iothub_deviceconfiguration_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    (void)umock_c_init(on_umock_c_error);

    int result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_TYPE(HTTPAPI_RESULT, HTTPAPI_RESULT);
    REGISTER_TYPE(HTTPAPIEX_RESULT, HTTPAPIEX_RESULT);
    REGISTER_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT);
    REGISTER_TYPE(HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE);
    REGISTER_UMOCK_ALIAS_TYPE(VECTOR_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const VECTOR_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PREDICATE_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_SAS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(JSON_Value_Type, int);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, 42);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct_n, my_STRING_construct_n);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct_n, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_from_byte_array, my_STRING_from_byte_array);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_from_byte_array, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_c_str, my_STRING_c_str);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_c_str, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_new, my_BUFFER_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_new, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_create, my_BUFFER_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, my_BUFFER_delete);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Alloc, my_HTTPHeaders_Alloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_Alloc, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Free, my_HTTPHeaders_Free);
    REGISTER_GLOBAL_MOCK_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_Create, my_HTTPAPIEX_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_Create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_Destroy, my_HTTPAPIEX_Destroy);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_SAS_Create, my_HTTPAPIEX_SAS_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_SAS_Create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_SAS_Destroy, my_HTTPAPIEX_SAS_Destroy);

    REGISTER_GLOBAL_MOCK_RETURN(HTTPAPIEX_SAS_ExecuteRequest, HTTPAPIEX_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_SAS_ExecuteRequest, HTTPAPIEX_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(json_value_init_object, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_init_object, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(json_value_get_object, TEST_JSON_OBJECT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_string, TEST_CONST_CHAR_PTR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_string, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_number, 42);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_number, -1);

    REGISTER_GLOBAL_MOCK_RETURN(json_object_dotget_string, TEST_CONST_CHAR_PTR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_dotget_string, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(json_object_set_string, TEST_JSON_STATUS);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_string, -1);

    REGISTER_GLOBAL_MOCK_RETURN(json_object_dotset_string, TEST_JSON_STATUS);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_dotset_string, -1);

    REGISTER_GLOBAL_MOCK_RETURN(json_parse_string, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_parse_string, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(json_serialize_to_string, TEST_CHAR_PTR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_serialize_to_string, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(json_array_get_array, TEST_JSON_ARRAY);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_array_get_array, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(json_array_get_object, TEST_JSON_OBJECT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_array_get_object, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(json_value_get_array, TEST_JSON_ARRAY);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_array, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(json_object_clear, JSONSuccess);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_clear, JSONFailure);

    REGISTER_GLOBAL_MOCK_RETURN(json_array_clear, JSONSuccess);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_array_clear, JSONFailure);

    REGISTER_GLOBAL_MOCK_RETURN(json_object_dotset_boolean, JSONSuccess);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_dotset_boolean, JSONFailure);

    REGISTER_GLOBAL_MOCK_RETURN(json_object_dotget_boolean, JSONSuccess);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_dotget_boolean, -1);

    REGISTER_GLOBAL_MOCK_RETURN(json_object_dotget_object, TEST_JSON_OBJECT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_dotget_object, NULL);

	REGISTER_GLOBAL_MOCK_RETURN(json_object_dotget_value, TEST_JSON_VALUE);
	REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_dotget_value, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_create, my_list_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_add, my_list_add);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_add, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_get_head_item, my_list_get_head_item);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_get_head_item, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_get_next_item, my_list_get_next_item);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_get_next_item, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_item_get_value, my_list_item_get_value);

    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_destroy, my_list_destroy);
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

    TEST_IOTHUB_SERVICE_CLIENT_AUTH.hostname = TEST_HOSTNAME;
    TEST_IOTHUB_SERVICE_CLIENT_AUTH.iothubName = TEST_IOTHUBNAME;
    TEST_IOTHUB_SERVICE_CLIENT_AUTH.iothubSuffix = TEST_IOTHUBSUFFIX;
    TEST_IOTHUB_SERVICE_CLIENT_AUTH.sharedAccessKey = TEST_SHAREDACCESSKEY;
    TEST_IOTHUB_SERVICE_CLIENT_AUTH.keyName = TEST_SHAREDACCESSKEYNAME;
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

/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_002: [ If any member of the serviceClientHandle input parameter is NULL IoTHubDeviceConfiguration_Create shall return NULL ]*/
TEST_FUNCTION(IoTHubDeviceConfiguration_Create_return_null_if_input_parameter_serviceClientHandle_hostName_is_NULL)
{
    // arrange
    TEST_IOTHUB_SERVICE_CLIENT_AUTH.hostname = NULL;

    // act
    IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE result = IoTHubDeviceConfiguration_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_002: [ If any member of the serviceClientHandle input parameter is NULL IoTHubDeviceConfiguration_Create shall return NULL ]*/
TEST_FUNCTION(IoTHubDeviceConfiguration_Create_return_null_if_input_parameter_serviceClientHandle_iothubName_is_NULL)
{
    // arrange
    TEST_IOTHUB_SERVICE_CLIENT_AUTH.iothubName = NULL;

    // act
    IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE result = IoTHubDeviceConfiguration_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_002: [ If any member of the serviceClientHandle input parameter is NULL IoTHubDeviceConfiguration_Create shall return NULL ]*/
TEST_FUNCTION(IoTHubDeviceConfiguration_Create_return_null_if_input_parameter_serviceClientHandle_iothubSuffix_is_NULL)
{
    // arrange
    TEST_IOTHUB_SERVICE_CLIENT_AUTH.iothubSuffix = NULL;

    // act
    IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE result = IoTHubDeviceConfiguration_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_002: [ If any member of the serviceClientHandle input parameter is NULL IoTHubDeviceConfiguration_Create shall return NULL ]*/
TEST_FUNCTION(IoTHubDeviceConfiguration_Create_return_null_if_input_parameter_serviceClientHandle_keyName_is_NULL)
{
    // arrange
    TEST_IOTHUB_SERVICE_CLIENT_AUTH.keyName = NULL;

    // act
    IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE result = IoTHubDeviceConfiguration_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_002: [ If any member of the serviceClientHandle input parameter is NULL IoTHubDeviceConfiguration_Create shall return NULL ]*/
TEST_FUNCTION(IoTHubDeviceConfiguration_Create_return_null_if_input_parameter_serviceClientHandle_sharedAccessKey_is_NULL)
{
    // arrange
    TEST_IOTHUB_SERVICE_CLIENT_AUTH.sharedAccessKey = NULL;

    // act
    IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE result = IoTHubDeviceConfiguration_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_003: [ IoTHubDeviceConfiguration_Create shall allocate memory for a new IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE instance ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_005: [ If the allocation successful, IoTHubDeviceConfiguration_Create shall create a IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE from the given IOTHUB_SERVICE_CLIENT_AUTH_HANDLE and return with it ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_006: [ IoTHubDeviceConfiguration_Create shall allocate memory and copy hostName to result-hostName by calling mallocAndStrcpy_s. ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_008: [ IoTHubDeviceConfiguration_Create shall allocate memory and copy iothubName to result->iothubName by calling mallocAndStrcpy_s. ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_010: [ IoTHubDeviceConfiguration_Create shall allocate memory and copy iothubSuffix to result->iothubSuffix by calling mallocAndStrcpy_s. ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_012: [ IoTHubDeviceConfiguration_Create shall allocate memory and copy sharedAccessKey to result->sharedAccessKey by calling mallocAndStrcpy_s. ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_014: [ IoTHubDeviceConfiguration_Create shall allocate memory and copy keyName to `result->keyName` by calling mallocAndStrcpy_s. ]*/
TEST_FUNCTION(IoTHubDeviceConfiguration_Create_happy_path)
{
    // arrange
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    // act
    IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE result = IoTHubDeviceConfiguration_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ASSERT_ARE_EQUAL(char_ptr, result->hostname, TEST_HOSTNAME);
    ASSERT_ARE_EQUAL(char_ptr, result->sharedAccessKey, TEST_SHAREDACCESSKEY);
    ASSERT_ARE_EQUAL(char_ptr, result->keyName, TEST_SHAREDACCESSKEYNAME);

    ///cleanup
    if (result != NULL)
    {
        free(result->hostname);
        free(result->keyName);
        free(result->sharedAccessKey);
        free(result);
        result = NULL;
    }
}

/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_004: [ If the allocation failed, IoTHubDeviceConfiguration_Create shall return NULL ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_007: [ If the mallocAndStrcpy_s fails, IoTHubDeviceConfiguration_Create shall do clean up and return NULL. ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_009: [ If the mallocAndStrcpy_s fails, IoTHubDeviceConfiguration_Create shall do clean up and return NULL. ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_011: [ If the mallocAndStrcpy_s fails, IoTHubDeviceConfiguration_Create shall do clean up and return NULL. ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_013: [ If the mallocAndStrcpy_s fails, IoTHubDeviceConfiguration_Create shall do clean up and return NULL. ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_015: [ If the mallocAndStrcpy_s fails, IoTHubDeviceConfiguration_Create shall do clean up and return NULL. ]*/
TEST_FUNCTION(IoTHubDeviceConfiguration_Create_non_happy_path)
{
    // arrange
    int umockc_result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, umockc_result);

    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, (const char*)(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE->hostname)))
        .IgnoreArgument(1);

    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, (const char*)(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE->iothubName)))
        .IgnoreArgument(1);

    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, (const char*)(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE->keyName)))
        .IgnoreArgument(1);

    umock_c_negative_tests_snapshot();

    ///act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        /// arrange
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        /// act
        IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE result = IoTHubDeviceConfiguration_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

        /// assert
        ASSERT_ARE_EQUAL(void_ptr, NULL, result);

        ///cleanup
    }
    umock_c_negative_tests_deinit();
}

/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_016: [ If the serviceClientDeviceConfigurationHandle input parameter is NULL IoTHubDeviceConfiguration_Destroy shall return ]*/
TEST_FUNCTION(IoTHubDeviceConfiguration_Destroy_return_if_input_parameter_serviceClientDeviceConfigurationHandle_is_NULL)
{
    // arrange
    
    // act
    IoTHubDeviceConfiguration_Destroy(NULL);
    
    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_017: [ If the serviceClientDeviceConfigurationHandle input parameter is not NULL IoTHubDeviceConfiguration_Destroy shall free the memory of it and return ]*/
TEST_FUNCTION(IoTHubDeviceConfiguration_Destroy_do_clean_up_and_return_if_input_parameter_serviceClientDeviceConfigurationHandle_is_not_NULL)
{
    // arrange
    IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE handle = IoTHubDeviceConfiguration_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    ASSERT_IS_NOT_NULL(handle);

    umock_c_reset_all_calls();

    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
    .IgnoreArgument(1);
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
    .IgnoreArgument(1);
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
    .IgnoreArgument(1);
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
    .IgnoreArgument(1);
    
    // act
    IoTHubDeviceConfiguration_Destroy(handle);
    
    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_018: [ IoTHubDeviceConfiguration_GetConfiguration shall verify the input parameters and if any of them are NULL then return IOTHUB_DEVICE_CONFIGURATION_INVALID_ARG ]*/
TEST_FUNCTION(IoTHubDeviceConfiguration_GetConfiguration_return_NULL_if_input_parameter_serviceClientDeviceConfigurationHandle_is_NULL)
{
    //arrange
    const char* configurationId = " ";

    //act
    IOTHUB_DEVICE_CONFIGURATION_RESULT result = IoTHubDeviceConfiguration_GetConfiguration(NULL, configurationId, (IOTHUB_DEVICE_CONFIGURATION*)0x4242);

    //assert
    ASSERT_ARE_EQUAL(int, IOTHUB_DEVICE_CONFIGURATION_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_018: [ IoTHubDeviceConfiguration_GetConfiguration shall verify the input parameters and if any of them are NULL then return IOTHUB_DEVICE_CONFIGURATION_INVALID_ARG ]*/
TEST_FUNCTION(IoTHubDeviceConfiguration_GetConfiguration_return_NULL_if_input_parameter_configurationId_is_NULL)
{
    // arrange
    IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE handle = IoTHubDeviceConfiguration_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    ASSERT_IS_NOT_NULL(handle);

    umock_c_reset_all_calls();

    // act
    IOTHUB_DEVICE_CONFIGURATION_RESULT result = IoTHubDeviceConfiguration_GetConfiguration(handle, NULL, (IOTHUB_DEVICE_CONFIGURATION*)0x4242);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_DEVICE_CONFIGURATION_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

static void set_expected_calls_for_sendHttpRequestDeviceConfiguration(const unsigned int httpStatusCode, HTTPAPI_REQUEST_TYPE requestType)
{
    EXPECTED_CALL(STRING_construct(TEST_HOSTNAME));
    EXPECTED_CALL(STRING_construct(TEST_SHAREDACCESSKEY));
    EXPECTED_CALL(STRING_construct(TEST_SHAREDACCESSKEYNAME));

    EXPECTED_CALL(HTTPHeaders_Alloc());
    EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, TEST_HTTP_HEADER_KEY_AUTHORIZATION, TEST_HTTP_HEADER_VAL_AUTHORIZATION))
        .IgnoreArgument(1);
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(UniqueId_Generate(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, TEST_HTTP_HEADER_KEY_REQUEST_ID, TEST_HTTP_HEADER_VAL_REQUEST_ID))
        .IgnoreArgument(1);
    EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, TEST_HTTP_HEADER_KEY_USER_AGENT, IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, TEST_HTTP_HEADER_KEY_ACCEPT, TEST_HTTP_HEADER_VAL_ACCEPT))
        .IgnoreArgument(1);
    EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, TEST_HTTP_HEADER_KEY_IFMATCH, TEST_HTTP_HEADER_VAL_IFMATCH))
        .IgnoreArgument(1);

    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    EXPECTED_CALL(HTTPAPIEX_SAS_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    EXPECTED_CALL(HTTPAPIEX_Create(TEST_HOSTNAME));

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));

    EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(IGNORED_PTR_ARG, IGNORED_PTR_ARG, requestType, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments()
        .CopyOutArgumentBuffer_statusCode(&httpStatusCode, sizeof(httpStatusCode))
        .SetReturn(HTTPAPIEX_OK);

    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(HTTPAPIEX_SAS_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
}

static void set_expected_calls_for_GetConfiguration_processing()
{
    /*EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));*/

    EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(TEST_UNSIGNED_CHAR_PTR);

    EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(TEST_JSON_VALUE); 
        
    STRICT_EXPECTED_CALL(json_value_get_object(TEST_JSON_VALUE))
        .SetReturn(TEST_JSON_OBJECT);

    /*STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_CONFIGURATION_JSON_KEY_CONFIGURATION_ID))
        .SetReturn(TEST_CONFIGURATION_ID);

    STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_CONFIGURATION_JSON_KEY_SCHEMA_VERSION))
        .SetReturn(TEST_CONFIGURATION_ID);

    STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_CONFIGURATION_JSON_KEY_CONTENT))
        .SetReturn(TEST_CONFIGURATION_ID);

    STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_DEVICE_JSON_KEY_CONFIGURATION_ID))
        .SetReturn(TEST_CONFIGURATION_ID);

    STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_DEVICE_JSON_KEY_CONFIGURATION_ID))
        .SetReturn(TEST_CONFIGURATION_ID);

    STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_DEVICE_JSON_KEY_CONFIGURATION_ID))
        .SetReturn(TEST_CONFIGURATION_ID);

    STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_DEVICE_JSON_KEY_CONFIGURATION_ID))
        .SetReturn(TEST_CONFIGURATION_ID);

    STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_DEVICE_JSON_KEY_CONFIGURATION_ID))
        .SetReturn(TEST_CONFIGURATION_ID);

    STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_DEVICE_JSON_KEY_CONFIGURATION_ID))
        .SetReturn(TEST_CONFIGURATION_ID);

    STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_DEVICE_JSON_KEY_CONFIGURATION_ID))
        .SetReturn(TEST_CONFIGURATION_ID);

    STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_DEVICE_JSON_KEY_CONFIGURATION_ID))
        .SetReturn(TEST_CONFIGURATION_ID);*/
    
    /*[json_object_get_string(0000000000005151, "id")]
    [json_object_get_string(0000000000005151, "schemaVersion")]
    [json_object_get_string(0000000000005151, "content")]
    [json_object_get_string(0000000000005151, "deviceContent")]
    [json_object_get_string(0000000000005151, "modulesContent")]
    [json_object_get_string(0000000000005151, "contentType")]
    ]json_object_get_string(0000000000005151, "targetCondition")]
    [json_object_get_string(0000000000005151, "createdTimeUtc")]
    [json_object_get_string(0000000000005151, "lastUpdatedTimeUtc")]
    [json_object_get_string(0000000000005151, "priority")]
    [json_object_get_string(0000000000005151, "metrics")]
    [json_object_get_string(0000000000005151, "results")]
    [json_object_get_string(0000000000005151, "queries")]
    [json_object_get_string(0000000000005151, "etag")]
    [mallocAndStrcpy_s(000001D65627BDE0, "TestConstChar")]
    [mallocAndStrcpy_s(000001D65627BDD8, "TestConstChar")]
    */
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

}

/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_019: [ IoTHubDeviceConfiguration_GetConfiguration shall create HTTP GET request URL using the given configurationId using the following format: url/configurations/[configurationId] ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_020: [ IoTHubDeviceConfiguration_GetConfiguration shall add the following headers to the created HTTP GET request: authorization=sasToken,Request-Id=<generatedGuid>,Accept=application/json,Content-Type=application/json,charset=utf-8 ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_021: [ IoTHubDeviceConfiguration_GetConfiguration shall create an HTTPAPIEX_SAS_HANDLE handle by calling HTTPAPIEX_SAS_Create ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_022: [ IoTHubDeviceConfiguration_GetConfiguration shall create an HTTPAPIEX_HANDLE handle by calling HTTPAPIEX_Create ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_023: [ IoTHubDeviceConfiguration_GetConfiguration shall execute the HTTP GET request by calling HTTPAPIEX_ExecuteRequest ]*/
/*Tests_SRS_IOTHUBDEVICECONFIGURATION_01_030: [ Otherwise IoTHubDeviceConfiguration_GetConfiguration shall save the received deviceTwin to the out parameter and return with it ]*/
TEST_FUNCTION(IoTHubDeviceConfiguration_GetConfiguration_happy_path_status_code_200)
{
    // arrange
    IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE handle = IoTHubDeviceConfiguration_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    ASSERT_IS_NOT_NULL(handle);

    umock_c_reset_all_calls();

    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(BUFFER_new());

    set_expected_calls_for_sendHttpRequestDeviceConfiguration(httpStatusCodeOk, HTTPAPI_REQUEST_GET);
    set_expected_calls_for_GetConfiguration_processing();

    // act
    IOTHUB_DEVICE_CONFIGURATION configuration;
    const char* configurationId = " ";
    IOTHUB_DEVICE_CONFIGURATION_RESULT result = IoTHubDeviceConfiguration_GetConfiguration(handle, configurationId, &configuration);
    (void)result;
    // assert
    //TODO: Fixup mock expected calls after integration is complete
    //ASSERT_ARE_EQUAL(int, IOTHUB_DEVICE_CONFIGURATION_OK, result);
    
	//TODO: Fixup mock expected calls after integration is complete
	//ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    //TODO: Readd after free() mock call sequence has been figured out
	//IoTHubDeviceConfiguration_FreeConfigurationMembers(&configuration);
}

/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_052: [ IoTHubDeviceConfiguration_DeleteConfiguration shall verify the input parameters and if any of them are NULL then return IOTHUB_DEVICE_CONFIGURATION_INVALID_ARG ] */
TEST_FUNCTION(IoTHubDeviceConfiguration_DeleteConfiguration_return_IOTHUB_DEVICECONFIGURATION_INVALID_ARG_if_input_parameter_configurationManagerHandle_is_NULL)
{
	///arrange

	///act
	IOTHUB_DEVICE_CONFIGURATION_RESULT result = IoTHubDeviceConfiguration_DeleteConfiguration(NULL, TEST_CONST_CHAR_PTR);

	///assert
	ASSERT_ARE_EQUAL(int, IOTHUB_DEVICE_CONFIGURATION_INVALID_ARG, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_052: [ IoTHubDeviceConfiguration_DeleteConfiguration shall verify the input parameters and if any of them are NULL then return IOTHUB_DEVICE_CONFIGURATION_INVALID_ARG ] */
TEST_FUNCTION(IoTHubDeviceConfiguration_DeleteConfiguration_return_IOTHUB_DEVICECONFIGURATION_INVALID_ARG_if_input_parameter_configurationId_is_NULL)
{
	///arrange
	IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE handle = IoTHubDeviceConfiguration_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
	ASSERT_IS_NOT_NULL(handle);

	umock_c_reset_all_calls();

	///act
	IOTHUB_DEVICE_CONFIGURATION_RESULT result = IoTHubDeviceConfiguration_DeleteConfiguration(handle, NULL);

	///assert
	ASSERT_ARE_EQUAL(int, IOTHUB_DEVICE_CONFIGURATION_INVALID_ARG, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_053: [ IoTHubDeviceConfiguration_DeleteConfiguration shall create HTTP DELETE request URL using the given configurationId using the following format : url/configurations/[configurationId]  ] */
/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_054: [ IoTHubDeviceConfiguration_DeleteConfiguration shall add the following headers to the created HTTP GET request : authorization=sasToken,Request-Id=<generatedGuid>,Accept=application/json,Content-Type=application/json,charset=utf-8 ] */
/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_055: [ IoTHubDeviceConfiguration_DeleteConfiguration shall create an HTTPAPIEX_SAS_HANDLE handle by calling HTTPAPIEX_SAS_Create ] */
/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_056: [ IoTHubDeviceConfiguration_DeleteConfiguration shall create an HTTPAPIEX_HANDLE handle by calling HTTPAPIEX_Create ] */
/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_057: [ IoTHubDeviceConfiguration_DeleteConfiguration shall execute the HTTP DELETE request by calling HTTPAPIEX_ExecuteRequest ] */
/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_058: [ IoTHubDeviceConfiguration_DeleteConfiguration shall verify the received HTTP status code and if it is greater than 300 then return IOTHUB_DEVICE_CONFIGURATION_HTTP_STATUS_ERROR ] */
/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_059: [ IoTHubDeviceConfiguration_DeleteConfiguration shall verify the received HTTP status code and if it is less or equal than 300 then return IOTHUB_DEVICE_CONFIGURATION_OK ] */
TEST_FUNCTION(IoTHubDeviceConfiguration_DeleteConfiguration_happy_path)
{
	///arrange
	IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE handle = IoTHubDeviceConfiguration_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
	ASSERT_IS_NOT_NULL(handle);

	umock_c_reset_all_calls();
	
	set_expected_calls_for_sendHttpRequestDeviceConfiguration(httpStatusCodeDeleted, HTTPAPI_REQUEST_DELETE);

	///act
	IOTHUB_DEVICE_CONFIGURATION_RESULT result = IoTHubDeviceConfiguration_DeleteConfiguration(handle, TEST_CONST_CHAR_PTR);

	///assert
	ASSERT_ARE_EQUAL(int, IOTHUB_DEVICE_CONFIGURATION_OK, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_053: [ IoTHubDeviceConfiguration_DeleteConfiguration shall create HTTP DELETE request URL using the given configurationId using the following format : url/configurations/[configurationId ] */
/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_054: [ IoTHubDeviceConfiguration_DeleteConfiguration shall add the following headers to the created HTTP GET request : authorization=sasToken,Request-Id=<generatedGuid>,Accept=application/json,Content-Type=application/json,charset=utf-8  ] */
/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_055: [ IoTHubDeviceConfiguration_DeleteConfiguration shall create an HTTPAPIEX_SAS_HANDLE handle by calling HTTPAPIEX_SAS_Create ] */
/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_056: [ IoTHubDeviceConfiguration_DeleteConfiguration shall create an HTTPAPIEX_HANDLE handle by calling HTTPAPIEX_Create ] */
/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_057: [ IoTHubDeviceConfiguration_DeleteConfiguration shall execute the HTTP DELETE request by calling HTTPAPIEX_ExecuteRequest ] */
/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_058: [ IoTHubDeviceConfiguration_DeleteConfiguration shall verify the received HTTP status code and if it is greater than 300 then return IOTHUB_DEVICE_CONFIGURATION_HTTP_STATUS_ERROR ] */
/* Tests_SRS_IOTHUBDEVICECONFIGURATION_01_059: [ IoTHubDeviceConfiguration_DeleteConfiguration shall verify the received HTTP status code and if it is less or equal than 300 then return IOTHUB_DEVICE_CONFIGURATION_OK ] */
TEST_FUNCTION(IoTHubDeviceConfiguration_DeleteConfiguration_non_happy_path)
{
	///arrange
	IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE handle = IoTHubDeviceConfiguration_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
	ASSERT_IS_NOT_NULL(handle);

	umock_c_reset_all_calls();
	
	int umockc_result = umock_c_negative_tests_init();
	ASSERT_ARE_EQUAL(int, 0, umockc_result);

	set_expected_calls_for_sendHttpRequestDeviceConfiguration(httpStatusCodeOk, HTTPAPI_REQUEST_DELETE);

	umock_c_negative_tests_snapshot();

	///act
	for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
	{
		/// arrange
		umock_c_negative_tests_reset();
		umock_c_negative_tests_fail_call(i);

		/// act
		if (
			(i != 6) && /*UniqueId_Generate*/
			(i != 11) && /*gballoc_free*/
			(i != 14) && /*STRING_c_str*/
			(i != 15) && /*HTTPAPIEX_SAS_Destroy*/
			(i != 16) && /*STRING_delete*/
			(i != 17) && /*HTTPAPIEX_Destroy*/
			(i != 18) && /*HTTPAPIEX_SAS_Destroy*/
			(i != 19) && /*HTTPHeaders_Free*/
			(i != 20) && /*STRING_delete*/
			(i != 21) && /*STRING_delete*/
			(i != 22)    /*STRING_delete*/
			)
		{
			IOTHUB_DEVICE_CONFIGURATION_RESULT result = IoTHubDeviceConfiguration_DeleteConfiguration(handle, TEST_CONST_CHAR_PTR);
			
			/// assert
			ASSERT_ARE_NOT_EQUAL(int, IOTHUB_DEVICE_CONFIGURATION_OK, result);
		}
		///cleanup
	}
	umock_c_negative_tests_deinit();
}

END_TEST_SUITE(iothub_deviceconfiguration_ut)
