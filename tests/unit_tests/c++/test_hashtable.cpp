/* -----------------------------------------------------------------------------
 *  (c) Crown copyright 2022 Met Office. All rights reserved.
 *  The file LICENCE, distributed with this code, contains details of the terms
 *  under which the code may be used.
 * -----------------------------------------------------------------------------
 */

#include <gmock/gmock.h>
#include <omp.h>

#include "profiler.h"

using ::testing::AllOf;
using ::testing::An;
using ::testing::Gt;

//
//  Testing that the hashing function works as expected (we don't want
//  collisions), and that walltimes are being updated by profiler.stop().
//  The desired behaviour of calling get_thread0_walltime before profiler.stop()
//  is a bit fuzzy at the time of writing, but currently a test is done to make
//  sure it returns the MDI of 0.0
//

TEST(HashTableTest,HashFunctionTest) {

  // Create new hashes via HashTable::query_insert, which is used in Profiler::start
  const auto& prof_rigatoni = prof.start("Rigatoni");
  const auto& prof_penne    = prof.start("Penne");
  prof.stop(prof_penne);
  prof.stop(prof_rigatoni);

  {
    SCOPED_TRACE("Hashing related fault");

    // Checking that:
    //  - query_insert'ing Penne or Rigatoni just returns the hash
    //  - the regions have different hashes
    //  - the regions have the hashes returned by hash_function_ which uses std::hash
    EXPECT_EQ(prof.start("Rigatoni"), std::hash<std::string_view>{}("Rigatoni"));
    EXPECT_EQ(prof.start("Penne"),    std::hash<std::string_view>{}("Penne"));
  }

}

/**
 * @TODO  Decide how to handle the MDI stuff and update the following test
 *        accordingly. See Issue #53.
 *
 */

TEST(HashTableTest,UpdateTimesTest) {

  // Create new hash
  size_t prof_pie = std::hash<std::string_view>{}("Pie");

  // Trying to find a time before .start() will throw an exception
  EXPECT_THROW(prof.get_thread0_walltime(prof_pie), std::out_of_range);

  // Start timing
  auto const& expected_hash = prof.start("Pie");
  EXPECT_EQ(expected_hash,prof_pie); // Make sure prof_pie has the hash we expect

  sleep(1);

  // Time t1 declared inbetween .start() and first .stop()
  double const t1 = prof.get_thread0_walltime(prof_pie);

  //Stop timing
  prof.stop(prof_pie);

  // Time t2 declared after first profiler.stop()
  double const t2 = prof.get_thread0_walltime(prof_pie);

  // Start and stop same region again
  prof.start("Pie");
  sleep(1);
  prof.stop(prof_pie);

  // Time t3 declared after second profiler.stop()
  double const t3 = prof.get_thread0_walltime(prof_pie);

  // Expected behaviour: t1 return the MDI and t3 > t2 > 0
  constexpr double MDI = 0.0;     // Missing Data Indicator (MDI)

  {
    SCOPED_TRACE("MDI missing from time points expected to return it");
    EXPECT_EQ(t1, MDI);
  }

  {
    SCOPED_TRACE("Update potentially not incrementing times correctly");
    EXPECT_GT(t2, 0.0);
    EXPECT_GT(t3, t2 );
  }
}
