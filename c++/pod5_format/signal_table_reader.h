#pragma once

#include "pod5_format/pod5_format_export.h"
#include "pod5_format/result.h"
#include "pod5_format/schema_metadata.h"
#include "pod5_format/signal_table_schema.h"
#include "pod5_format/table_reader.h"
#include "pod5_format/types.h"

#include <arrow/io/type_fwd.h>
#include <boost/optional.hpp>
#include <boost/uuid/uuid.hpp>
#include <gsl/gsl-lite.hpp>

#include <atomic>

namespace arrow {
class Schema;
namespace io {
class RandomAccessFile;
}
namespace ipc {
class RecordBatchFileReader;
}
}  // namespace arrow

namespace pod5 {

class POD5_FORMAT_EXPORT SignalTableRecordBatch : public TableRecordBatch {
public:
    SignalTableRecordBatch(std::shared_ptr<arrow::RecordBatch>&& batch,
                           SignalTableSchemaDescription field_locations,
                           arrow::MemoryPool* pool);

    std::shared_ptr<UuidArray> read_id_column() const;
    std::shared_ptr<arrow::LargeListArray> uncompressed_signal_column() const;
    std::shared_ptr<VbzSignalArray> vbz_signal_column() const;
    std::shared_ptr<arrow::UInt32Array> samples_column() const;

    Result<std::size_t> samples_byte_count(std::size_t row_index) const;

    /// \brief Extract a row of sample data into [samples], decompressing if required.
    Status extract_signal_row(std::size_t row_index, gsl::span<std::int16_t> samples) const;

private:
    SignalTableSchemaDescription m_field_locations;
    arrow::MemoryPool* m_pool;
};

class POD5_FORMAT_EXPORT SignalTableReader : public TableReader {
public:
    SignalTableReader(std::shared_ptr<void>&& input_source,
                      std::shared_ptr<arrow::ipc::RecordBatchFileReader>&& reader,
                      std::vector<pod5::SignalTableRecordBatch>&& table_batches,
                      SignalTableSchemaDescription field_locations,
                      SchemaMetadataDescription&& schema_metadata,
                      arrow::MemoryPool* pool);

    SignalTableReader(SignalTableReader&&);
    SignalTableReader& operator=(SignalTableReader&&);

    Result<SignalTableRecordBatch> read_record_batch(std::size_t i) const;

    Result<std::size_t> signal_batch_for_row_id(std::uint64_t row,
                                                std::size_t* batch_start_row) const;

    /// \brief Find the number of samples in a given list of rows.
    /// \param row_indices      The rows to query for sample ount.
    /// \returns The sum of all sample counts on input rows.
    Result<std::size_t> extract_sample_count(
            gsl::span<std::uint64_t const> const& row_indices) const;

    /// \brief Extract the samples for a list of rows.
    /// \param row_indices      The rows to query for samples.
    /// \param output_samples   The output samples from the rows. Data in the vector is cleared before appending.
    Status extract_samples(gsl::span<std::uint64_t const> const& row_indices,
                           gsl::span<std::int16_t> const& output_samples) const;

private:
    SignalTableSchemaDescription m_field_locations;
    arrow::MemoryPool* m_pool;

    std::vector<pod5::SignalTableRecordBatch> m_table_batches;

    // Cached size of the standard signal table batch size (true for all batches except last).
    mutable std::atomic<std::size_t> m_batch_size;
};

POD5_FORMAT_EXPORT Result<SignalTableReader> make_signal_table_reader(
        std::shared_ptr<arrow::io::RandomAccessFile> const& sink,
        arrow::MemoryPool* pool);

}  // namespace pod5