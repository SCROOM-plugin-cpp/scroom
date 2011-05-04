/*
 * Scroom - Generic viewer for 2D data
 * Copyright (C) 2009-2011 Kees-Jan Dijkzeul
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License, version 2, as published by the Free Software Foundation.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <scroom/threadpool.hh>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>

#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>

#include <scroom/semaphore.hh>

#include "helpers.hh"

#include "queue.hh"

using namespace boost::posix_time;
using namespace Scroom::Detail::ThreadPool;

const millisec short_timeout(250);
const millisec long_timeout(2000);

//////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(ThreadPool_Queue_Tests)

BOOST_AUTO_TEST_SUITE(Queue_Tests)

BOOST_AUTO_TEST_CASE(basic_jobcounting)
{
  QueueImpl::Ptr queue = QueueImpl::create();
  BOOST_CHECK(queue);
  BOOST_CHECK_EQUAL(0, queue->getCount());
  queue->jobStarted();
  BOOST_CHECK_EQUAL(1, queue->getCount());
  queue->jobStarted();
  BOOST_CHECK_EQUAL(2, queue->getCount());
  queue->jobFinished();
  BOOST_CHECK_EQUAL(1, queue->getCount());
  queue->jobFinished();
  BOOST_CHECK_EQUAL(0, queue->getCount());
}

BOOST_AUTO_TEST_CASE(destroy_waits_for_jobs_to_finish)
{
  ThreadPool::Queue::Ptr queue = ThreadPool::Queue::create();
  ThreadPool::Queue::WeakPtr weakQueue = queue;
  QueueImpl::Ptr qi = queue->get();
  BOOST_CHECK(queue);
  BOOST_CHECK(qi);
  BOOST_CHECK_EQUAL(0, qi->getCount());
  qi->jobStarted();
  BOOST_CHECK_EQUAL(1, qi->getCount());
  qi->jobStarted();
  BOOST_CHECK_EQUAL(2, qi->getCount());

  Semaphore s0(0);
  Semaphore s1(0);
  Semaphore s2(0);
  boost::thread t(boost::bind(pass_destroy_and_clear, &s0, &s1, &s2, weakQueue));
  s0.P();
  BOOST_REQUIRE(!s2.P(short_timeout));
  queue.reset();
  BOOST_CHECK(weakQueue.lock());
  s1.V();
  BOOST_REQUIRE(!s2.P(long_timeout));
  BOOST_CHECK(!weakQueue.lock());

  // At this point, all references to ThreadPool::Queue are gone, but the thread
  // trying to destroy it is blocked because
  // not all jobs have finished yet. So we should report the jobs complete,
  // and then the thread will unblock and the object will actually be deleted.
  qi->jobFinished();
  BOOST_REQUIRE(!s2.P(short_timeout));
  BOOST_CHECK_EQUAL(1, qi->getCount());
  qi->jobFinished();
  BOOST_REQUIRE(s2.P(long_timeout));
}

BOOST_AUTO_TEST_CASE(destroy_using_QueueLock)
{
  ThreadPool::Queue::Ptr queue = ThreadPool::Queue::create();
  ThreadPool::Queue::WeakPtr weakQueue = queue;
  BOOST_CHECK(queue);
  QueueLock* l = new QueueLock(queue->get());

  Semaphore s0(0);
  Semaphore s1(0);
  Semaphore s2(0);
  boost::thread t(boost::bind(pass_destroy_and_clear, &s0, &s1, &s2, weakQueue));
  s0.P();
  BOOST_REQUIRE(!s2.P(short_timeout));
  queue.reset();
  BOOST_CHECK(weakQueue.lock());
  s1.V();
  BOOST_REQUIRE(!s2.P(long_timeout));
  BOOST_CHECK(!weakQueue.lock());

  // At this point, all references to ThreadPool::Queue are gone, but the thread
  // trying to destroy it is blocked because
  // not all jobs have finished yet. So we should report the jobs complete,
  // and then the thread will unblock and the object will actually be deleted.
  delete l;
  BOOST_REQUIRE(s2.P(long_timeout));
}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Queue_Tests)

BOOST_AUTO_TEST_CASE(jobs_on_custom_queue_get_executed)
{
  ThreadPool::Queue::Ptr queue = ThreadPool::Queue::create();
  Semaphore s(0);
  ThreadPool t(0);
  t.schedule(boost::bind(clear_sem, &s), queue);
  t.add();
  BOOST_CHECK(s.P(long_timeout));
}

BOOST_AUTO_TEST_CASE(jobs_on_deleted_queue_dont_get_executed)
{
  ThreadPool::Queue::Ptr queue = ThreadPool::Queue::create();
  Semaphore s(0);
  ThreadPool t(0);
  t.schedule(boost::bind(clear_sem, &s), queue);
  queue.reset();
  t.add();
  BOOST_CHECK(!s.P(long_timeout));
}

BOOST_AUTO_TEST_CASE(queue_deletion_waits_for_jobs_to_finish)
{
  ThreadPool::Queue::Ptr queue = ThreadPool::Queue::create();
  ThreadPool::Queue::WeakPtr weakQueue = queue;
  Semaphore s0(0);
  Semaphore s1(0);
  Semaphore s2(0);
  Semaphore s3(0);
  Semaphore s4(0);

  ThreadPool pool(0);
  pool.schedule(boost::bind(clear_and_pass, &s1, &s2), queue);
  pool.add();
  BOOST_REQUIRE(s1.P(long_timeout));
  // Job is now being executed, hence it should not be possible to delete the queue

  // Setup: Create a thread that will delete the queue. Then delete our
  // reference, because if our reference is the last, our thread will block,
  // resulting in deadlock
  boost::thread t(boost::bind(pass_destroy_and_clear, &s0, &s3, &s4, weakQueue));
  s0.P();
  BOOST_CHECK(!s4.P(short_timeout));
  queue.reset();

  // Tell the thread to start deleting the Queue
  s3.V();
  // Thread does not finish
  BOOST_CHECK(!s4.P(long_timeout));

  // Complete the job
  s2.V();
  // Thread now finishes throwing away the Queue
  BOOST_CHECK(s4.P(long_timeout));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()