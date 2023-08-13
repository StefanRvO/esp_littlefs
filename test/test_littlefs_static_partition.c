//#define LOG_LOCAL_LEVEL 4
#include "test_littlefs_common.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "spi_flash_mmap.h"
#else
#include "esp_spi_flash.h"
#endif

#include "esp_flash.h"


static esp_partition_t get_test_data_static_partition(void);
static void test_setup_static(const esp_partition_t* partition);
static void test_teardown_static(const esp_partition_t* partition);

void test_setup_static(const esp_partition_t* partition) {
    const esp_vfs_littlefs_conf_partition_t conf = {
        .base_path = littlefs_base_path,
        .partition = partition,
        .dont_mount = false,
    };

    TEST_ESP_OK(esp_vfs_littlefs_register_partition(&conf));
    TEST_ASSERT_TRUE( heap_caps_check_integrity_all(true) );
    printf("Test setup complete.\n");
}

void test_teardown_static(const esp_partition_t* partition) {
    TEST_ESP_OK(esp_vfs_littlefs_unregister_partition(partition));
    TEST_ASSERT_TRUE( heap_caps_check_integrity_all(true) );
    printf("Test teardown complete.\n");
}

esp_partition_t get_test_data_static_partition(void)
{
    /* This finds the static partition embedded in the firmware and constructs a partition struct */
    extern const uint8_t partition_blob_start[] asm("_binary_testfs_bin_start");
    extern const uint8_t partition_blob_end[] asm("_binary_testfs_bin_end");

    return (esp_partition_t){
        .flash_chip = esp_flash_default_chip,
        .type       = ESP_PARTITION_TYPE_DATA,
        .subtype    = ESP_PARTITION_SUBTYPE_DATA_FAT,
        .address    = spi_flash_cache2phys(partition_blob_start),
        .size       = ((uint32_t)partition_blob_end) - ((uint32_t)partition_blob_start),
        .label      = "",
        .encrypted  = false,
    };
}


TEST_CASE("can read embedded partition", "[littlefs_static]")
{
    const esp_partition_t partition = get_test_data_static_partition();
    test_setup_static(&partition);
    size_t total = 0, used = 0;
    TEST_ESP_OK(esp_littlefs_partition_info(&partition, &total, &used));
    printf("total: %d, used: %d\n", total, used);
    TEST_ASSERT_EQUAL(20480, used); // The test FS has 5 used blocks of 4K
    test_teardown_static(&partition);
}
