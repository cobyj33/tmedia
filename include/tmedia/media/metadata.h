#ifndef TMEDIA_METADATA_H
#define TMEDIA_METADATA_H

#include <string>
#include <filesystem> // for std::filesystem::path
#include <string_view>
#include <vector>
#include <optional>

extern "C" {
struct AVFormatContext;
}

/**
 * Each Metadata Cache is implemented as a simple array of key-value pairs,
 * mostly because it was the simplest implementation I could think of, and it
 * lended itself well to using std::string_views in file and key lookup. Since
 * all use-cases of Metadata currently have extremely low entry counts, no
 * hashtable or std::map is necessary.
 * 
 * Note that if any functions take in a std::string_view as a file parameter
 * and the caller is using a std::filesystem::path object to store their path,
 * then the caller should use the return value of
 * std::filesystem::path.c_str() as the file key.
 */

struct MetadataEntry {
  std::string key;
  std::string value;
};

typedef std::vector<MetadataEntry> Metadata;

/**
 * Fill a given metadata dictionary with the metadata information present
 * on an AVFormatContext instance.
*/
void fmt_ctx_meta(AVFormatContext* fmt_ctx, Metadata& outdict);
std::string_view metadata_add(std::string_view key, std::string_view value, Metadata& meta);

/**
 * Note that mchc_get should be used to query if a key on a metadata instance
 * exists by checking if the returned value is std::nullopt or not. There is
 * no separate metadata_has for this operation.
 */
std::optional<std::string_view> metadata_get(std::string_view key, const Metadata& meta);

struct MetadataCacheEntry {
  std::string key;
  std::vector<MetadataEntry> value;
};

/**
 * A general cache-type map between filenames or some other media id strings
 * and metadata maps.
*/
typedef std::vector<MetadataCacheEntry> MetadataCache;

/**
 * Cache the metadata from a specific file into the metadata cache. This is
 * done by opening an AVFormatContext on the given media file path and adding
 * its metadata into the cache. However, if the file is already in the metadata
 * cache, then nothing is done. This makes mchc_cache_file safe to call multiple
 * times with the same metadata cache and file without excessive expensive
 * operations taking place.
 */
void mchc_cache_file(const std::filesystem::path& file, MetadataCache& cache);

/**
 * Add a key-value pair to the file specified in the metadata cache.
 * 
 * If the given key is already present on the metadata attached to the given
 * file in the metadata cache, then the previous file-key-value trio is replaced
 * with the new file-key-value trio.
 */
std::string_view mchc_add(std::string_view file, std::string_view key, std::string_view val, MetadataCache& cache);

/**
 * Check if the given metadata cache contains the given file.
*/
bool mchc_has_file(std::string_view file, const MetadataCache& cache);

/**
 * Get a metadata value for a specific cached value
 * 
 * @returns std::nullopt if the given key on the given file cannot be found.
 * A std::string_view of the metadata value if it could be found
 * 
 * Note that mchc_get should be used to query if a key for a file exists as
 * well, just by checking if the returned value is std::nullopt or not.
 */
std::optional<std::string_view> mchc_get(std::string_view file, std::string_view key, const MetadataCache& cache);


#endif