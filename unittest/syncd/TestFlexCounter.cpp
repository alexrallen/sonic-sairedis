#include "FlexCounter.h"
#include "MockableSaiInterface.h"
#include "MockHelper.h"

#include <gtest/gtest.h>


using namespace syncd;
using namespace std;

TEST(FlexCounter, addRemoveCounterForFlowCounter)
{
    std::shared_ptr<MockableSaiInterface> sai(new MockableSaiInterface());
    FlexCounter fc("test", sai, "COUNTERS_DB");

    sai_object_id_t counterVid{100};
    sai_object_id_t counterRid{100};
    std::vector<swss::FieldValueTuple> values;
    values.emplace_back(FLOW_COUNTER_ID_LIST, "SAI_COUNTER_STAT_PACKETS,SAI_COUNTER_STAT_BYTES");

    test_syncd::mockVidManagerObjectTypeQuery(SAI_OBJECT_TYPE_COUNTER);
    sai->mock_getStatsExt = [](sai_object_type_t, sai_object_id_t, uint32_t number_of_counters, const sai_stat_id_t *, sai_stats_mode_t, uint64_t *counters) {
        for (uint32_t i = 0; i < number_of_counters; i++)
        {
            counters[i] = (i + 1) * 100;
        }
        return SAI_STATUS_SUCCESS;
    };

    fc.addCounter(counterVid, counterRid, values);
    EXPECT_EQ(fc.isEmpty(), false);

    values.clear();
    values.emplace_back(POLL_INTERVAL_FIELD, "1000");
    fc.addCounterPlugin(values);

    values.clear();
    values.emplace_back(FLEX_COUNTER_STATUS_FIELD, "enable");
    fc.addCounterPlugin(values);

    usleep(1000*1000);
    swss::DBConnector db("COUNTERS_DB", 0);
    swss::RedisPipeline pipeline(&db);
    swss::Table countersTable(&pipeline, COUNTERS_TABLE, false);

    std::vector<std::string> keys;
    countersTable.getKeys(keys);
    EXPECT_EQ(keys.size(), size_t(1));
    EXPECT_EQ(keys[0], "oid:0x64");

    std::string value;
    countersTable.hget("oid:0x64", "SAI_COUNTER_STAT_PACKETS", value);
    EXPECT_EQ(value, "100");
    countersTable.hget("oid:0x64", "SAI_COUNTER_STAT_BYTES", value);
    EXPECT_EQ(value, "200");

    fc.removeCounter(counterVid);
    EXPECT_EQ(fc.isEmpty(), true);
    countersTable.getKeys(keys);

    ASSERT_TRUE(keys.empty());

}

TEST(FlexCounter, addRemoveCounterPluginForFlowCounter)
{
    std::shared_ptr<MockableSaiInterface> sai(new MockableSaiInterface());
    FlexCounter fc("test", sai, "COUNTERS_DB");

    std::vector<swss::FieldValueTuple> values;
    values.emplace_back(FLOW_COUNTER_PLUGIN_FIELD, "dummy_sha_strings");
    fc.addCounterPlugin(values);
    EXPECT_EQ(fc.isEmpty(), false);

    fc.removeCounterPlugins();
    EXPECT_EQ(fc.isEmpty(), true);
}
