// #define LOG_LOCAL_LEVEL 4
#include "test_littlefs_common.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  #include "spi_flash_mmap.h"
#else
  #include "esp_spi_flash.h"
#endif

#include "esp_flash.h"

static esp_partition_t get_test_data_static_partition(void);
static void            test_setup_static(const esp_partition_t* partition);
static void            test_teardown_static(const esp_partition_t* partition);

void test_setup_static(const esp_partition_t* partition)
{
  const esp_vfs_littlefs_conf_partition_t conf = {
    .base_path  = littlefs_base_path,
    .partition  = partition,
    .dont_mount = false,
  };

  TEST_ESP_OK(esp_vfs_littlefs_register_partition(&conf));
  TEST_ASSERT_TRUE(heap_caps_check_integrity_all(true));
  printf("Test setup complete.\n");
}

void test_teardown_static(const esp_partition_t* partition)
{
  TEST_ESP_OK(esp_vfs_littlefs_unregister_partition(partition));
  TEST_ASSERT_TRUE(heap_caps_check_integrity_all(true));
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

TEST_CASE("can read partition", "[littlefs_static]")
{
  const esp_partition_t partition = get_test_data_static_partition();
  test_setup_static(&partition);
  size_t total = 0, used = 0;
  TEST_ESP_OK(esp_littlefs_partition_info(&partition, &total, &used));
  printf("total: %d, used: %d\n", total, used);
  TEST_ASSERT_EQUAL(20480, used);  // The test FS has 5 used blocks of 4K
  test_teardown_static(&partition);
}

TEST_CASE("can read file", "[littlefs_static]")
{
  const esp_partition_t partition = get_test_data_static_partition();
  test_setup_static(&partition);

  test_littlefs_read_file_with_content(littlefs_base_path "/test1.txt", "test1");
  test_littlefs_read_file_with_content(littlefs_base_path "/test2.txt", "test2");
  test_littlefs_read_file_with_content(littlefs_base_path "/pangram.txt", "The quick brown fox jumps over the lazy dog");
  test_littlefs_read_file_with_content(littlefs_base_path "/test_folder/lorem_ipsum.txt",
    "Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod tincidunt ut laoreet "
    "dolore magna aliquam erat volutpat. Ut wisi enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit "
    "lobortis nisl ut aliquip ex ea commodo consequat. Duis autem vel eum iriure dolor in hendrerit in vulputate velit "
    "esse molestie consequat, vel illum dolore eu feugiat nulla facilisis at vero eros et accumsan et iusto odio "
    "dignissim qui blandit praesent luptatum zzril delenit augue duis dolore te feugait nulla facilisi. Nam liber "
    "tempor cum soluta nobis eleifend option congue nihil imperdiet doming id quod mazim placerat facer possim assum. "
    "Typi non habent claritatem insitam; est usus legentis in iis qui facit eorum claritatem. Investigationes "
    "demonstraverunt lectores legere me lius quod ii legunt saepius. Claritas est etiam processus dynamicus, qui "
    "sequitur mutationem consuetudium lectorum. Mirum est notare quam littera gothica, quam nunc putamus parum claram, "
    "anteposuerit litterarum formas humanitatis per seacula quarta decima et quinta decima. Eodem modo typi, qui nunc "
    "nobis videntur parum clari, fiant sollemnes in futurum.");

  test_teardown_static(&partition);
}
