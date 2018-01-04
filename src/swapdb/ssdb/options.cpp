/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "options.h"
#include "util/strings.h"
#include "util/config.h"


void Options::load(Config *conf) {
    if (conf == nullptr) {
        return;
    }
    c = conf;


    cache_size = (size_t) conf->get_num("rocksdb.cache_size", 16);
    sim_cache = (size_t) conf->get_num("rocksdb.sim_cache", 0);
    block_size = (size_t) conf->get_num("rocksdb.block_size", 16);

    max_open_files = conf->get_num("rocksdb.max_open_files", 1000);
    write_buffer_size = conf->get_num("rocksdb.write_buffer_size", 16);

    compression = conf->get_bool("rocksdb.compression");
    rdb_compression = conf->get_bool("rocksdb.rdb_compression", false);
    transfer_compression = conf->get_bool("rocksdb.transfer_compression");
    level_compaction_dynamic_level_bytes = conf->get_bool("rocksdb.level_compaction_dynamic_level_bytes", false);
    use_direct_reads = conf->get_bool("rocksdb.use_direct_reads", false);
    optimize_filters_for_hits = conf->get_bool("rocksdb.optimize_filters_for_hits", false);
    cache_index_and_filter_blocks = conf->get_bool("rocksdb.cache_index_and_filter_blocks", false);

    compaction_readahead_size = (size_t) conf->get_num("rocksdb.compaction_readahead_size", 4);
    max_bytes_for_level_base = (size_t) conf->get_num("rocksdb.max_bytes_for_level_base", 256);
    max_bytes_for_level_multiplier = (size_t) conf->get_num("rocksdb.max_bytes_for_level_multiplier", 10);

    target_file_size_base = (size_t) conf->get_num("rocksdb.target_file_size_base", 64);

    min_write_buffer_number_to_merge = conf->get_num("rocksdb.min_write_buffer_number_to_merge", 2);
    max_write_buffer_number = conf->get_num("rocksdb.max_write_buffer_number", 3);
    max_background_flushes = conf->get_num("rocksdb.max_background_flushes", 4);
    max_background_compactions = conf->get_num("rocksdb.max_background_compactions", 4);

    level0_file_num_compaction_trigger = conf->get_num("rocksdb.level0_file_num_compaction_trigger", 4);
    level0_slowdown_writes_trigger = conf->get_num("rocksdb.level0_slowdown_writes_trigger", 20);
    level0_stop_writes_trigger = conf->get_num("rocksdb.level0_stop_writes_trigger", 36);


}

std::ostream &operator<<(std::ostream &os, const Options &options) {
    os
            << "\n\t ============================Config=================================="
            << "\n\t| create_if_missing: " << options.create_if_missing
            << "\n\t| create_missing_column_families: " << options.create_missing_column_families
            << "\n\t| write_buffer_size: " << options.write_buffer_size
            << "\n\t| max_open_files: " << options.max_open_files
            << "\n\t| compression: " << options.compression
            << "\n\t| rdb_compression: " << options.rdb_compression
            << "\n\t| transfer_compression: " << options.transfer_compression
            << "\n\t| level_compaction_dynamic_level_bytes: " << options.level_compaction_dynamic_level_bytes
            << "\n\t| use_direct_reads: " << options.use_direct_reads
            << "\n\t| optimize_filters_for_hits: " << options.optimize_filters_for_hits
            << "\n\t| max_write_buffer_number: " << options.max_write_buffer_number
            << "\n\t| max_background_flushes: " << options.max_background_flushes
            << "\n\t| max_background_compactions: " << options.max_background_compactions
            << "\n\t| sim_cache: " << options.sim_cache
            << "\n\t| cache_size: " << options.cache_size
            << "\n\t| block_size: " << options.block_size
            << "\n\t| compaction_readahead_size: " << options.compaction_readahead_size
            << "\n\t| max_bytes_for_level_base: " << options.max_bytes_for_level_base
            << "\n\t| max_bytes_for_level_multiplier: " << options.max_bytes_for_level_multiplier
            << "\n\t| target_file_size_base: " << options.target_file_size_base
            << "\n\t| level0_file_num_compaction_trigger: " << options.level0_file_num_compaction_trigger
            << "\n\t| level0_slowdown_writes_trigger: " << options.level0_slowdown_writes_trigger
            << "\n\t| level0_stop_writes_trigger: " << options.level0_stop_writes_trigger
            << "\n\t ============================Config=================================="
            ;

    return os;
}
