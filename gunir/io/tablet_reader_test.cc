// Copyright (C) 2015. The Gunir Authors. All rights reserved.
// Author: An Qin (anqin.qin@gmail.com)
//

#include <vector>

#include "toft/storage/file/file.h"
#include "toft/storage/recordio/recordio.h"
#include "toft/system/memory/mempool.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/gtest/gtest.h"

#include "gunir/io/column_metadata.pb.h"
#include "gunir/io/io_test_helper.h"
#include "gunir/io/slice.h"
#include "gunir/io/table_builder.h"
#include "gunir/io/tablet_reader.h"
#include "gunir/io/tablet_schema.pb.h"
#include "gunir/io/testdata/document.pb.h"
#include "gunir/utils/proto_message.h"

namespace gunir {
namespace io {

class TabletReaderTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        IOTestHelper helper;
        helper.BuildTabletTestFile("Document", &m_output);

        m_mempool = new MemPool(MemPool::MAX_UNIT_SIZE);
        m_tablet_reader = new TabletReader(m_mempool);
    }
    virtual void TearDown() {
        delete m_tablet_reader;
        delete m_mempool;
    }

protected:
    MemPool* m_mempool;
    TabletReader *m_tablet_reader;
    std::vector<std::string> m_output;
};

// The document.col_0 is the tablet file generated by TableBuidler in
// SetUp(). Thus this test would depond on TabletWriter, ColumnWriter,
// ColumnReader and TableBuilder.
TEST_F(TabletReaderTest, Init) {
    EXPECT_TRUE(m_tablet_reader->Init(m_output[0]));
}

TEST_F(TabletReaderTest, Close) {
    EXPECT_TRUE(m_tablet_reader->Init(m_output[0]));
    EXPECT_TRUE(m_tablet_reader->Close());
}

TEST_F(TabletReaderTest, Read) {
    EXPECT_TRUE(m_tablet_reader->Init(m_output[0]));

    uint32_t count = 6;
    uint32_t slice_num = 0;

    while (m_tablet_reader->Next()) {
        Slice* slice = m_tablet_reader->GetSlice();
        EXPECT_EQ(slice->GetCount(), count);

//         LOG(ERROR) << "\nnew slice: " << slice_num;
        for (uint32_t i = 0; i < count; ++i) {
            const Block*  block = slice->GetBlock(i);
            if (slice->HasBlock(i)) {
                CHECK_NOTNULL(block);
            }
        }

        slice_num++;
    }

    uint32_t slice_count = 60;
    EXPECT_EQ(slice_num, slice_count);
    EXPECT_FALSE(m_tablet_reader->Next());
    EXPECT_TRUE(m_tablet_reader->Close());
}

TEST_F(TabletReaderTest, GetTabletSchema) {
    EXPECT_TRUE(m_tablet_reader->Init(m_output[0]));

    TabletSchema ts;
    m_tablet_reader->GetTabletSchema(&ts);

    EXPECT_EQ(ts.table_name(), "Document");

    std::string content;
    ProtoMessage proto_message;
    proto_message.CreateMessageByProtoFile("testdata/document.proto",
                                           "Document");
    content = proto_message.GetFileDescriptorSetString();
    EXPECT_EQ(ts.schema_descriptor().description(), content);
    EXPECT_EQ(ts.schema_descriptor().record_name(), "Document");

    EXPECT_EQ(ts.column_metadatas_size(), 6);
    EXPECT_TRUE(m_tablet_reader->Close());
}
}  // namespace io
}  // namespace gunir
