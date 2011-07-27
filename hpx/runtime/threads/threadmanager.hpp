//  Copyright (c) 2007-2009 Chirag Dekate, Hartmut Kaiser, Anshul Tandon
//  Copyright (c)      2011 Bryce Lelbach, Katelyn Kufahl
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_THREADMANAGER_MAY_20_2008_845AM)
#define HPX_THREADMANAGER_MAY_20_2008_845AM

#include <map>
#include <vector>
#include <memory>
#include <numeric>

#include <hpx/config.hpp>

#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/lockfree/fifo.hpp>
#include <boost/atomic.hpp>
#include <boost/cstdint.hpp>

#include <hpx/hpx_fwd.hpp>
#include <hpx/state.hpp>
#include <hpx/exception.hpp>
#include <hpx/runtime/naming/name.hpp>
#include <hpx/runtime/threads/thread_init_data.hpp>
#include <hpx/util/io_service_pool.hpp>
#include <hpx/util/block_profiler.hpp>
#include <hpx/util/high_resolution_timer.hpp>
#include <hpx/util/spinlock.hpp>

#include <hpx/config/warnings_prefix.hpp>

// TODO: add branch prediction and function heat

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace threads
{
    struct register_thread_tag {};
    struct register_work_tag {};
    struct set_state_tag {};

    ///////////////////////////////////////////////////////////////////////////
    /// \class threadmanager_base threadmanager.hpp hpx/runtime/threads/threadmanager.hpp
    struct threadmanager_base : private boost::noncopyable
    {
    protected:
        // we use the boost::posix_time::ptime type for time representation
        typedef boost::posix_time::ptime time_type;

        // we use the boost::posix_time::time_duration type as the duration 
        // representation
        typedef boost::posix_time::time_duration duration_type;

    public:
        virtual ~threadmanager_base() {}

        /// \brief Return whether the thread manager is still running
        virtual state status() const = 0;

        /// \brief return the number of PX-threads with the given state
        virtual boost::int64_t get_thread_count(
            thread_state_enum state = unknown) const = 0;

        // \brief Abort all threads which are in suspended state. This will set 
        //        the state of all suspended threads to \a pending while 
        //        supplying the wait_abort extended state flag
        virtual void abort_all_suspended_threads() = 0;

        // \brief Clean up terminated threads. This deletes all threads which 
        //        have been terminated but which are still held in the queue 
        //        of terminated threads. Some schedulers might not do anything
        //        here.
        virtual bool cleanup_terminated() = 0;
        
        /// The get_phase function is part of the thread related API. It 
        /// queries the phase of one of the threads known to the threadmanager
        ///
        /// \param id       [in] The thread id of the thread the phase should 
        ///                 be returned for.
        ///
        /// \returns        This function returns the current phase of the 
        ///                 thread referenced by the \a id parameter. If the
        ///                 thread is not known to the threadmanager the return 
        ///                 value will be ~0. 
        virtual std::size_t get_phase(thread_id_type id) = 0;

        /// The get_state function is part of the thread related API. It
        /// queries the state of one of the threads known to the threadmanager
        ///
        /// \param id       [in] The thread id of the thread the state should 
        ///                 be returned for.
        ///
        /// \returns        This function returns the current state of the 
        ///                 thread referenced by the \a id parameter. It will 
        ///                 return one of the values as defined by the 
        ///                 \a thread_state enumeration. If the 
        ///                 thread is not known to the threadmanager the return 
        ///                 value will be \a thread_state#unknown.
        virtual thread_state get_state(thread_id_type id) = 0;

        /// The set_state function is part of the thread related API and allows
        /// to change the state of one of the threads managed by this 
        /// threadmanager.
        ///
        /// \param id       [in] The thread id of the thread the state should 
        ///                 be modified for.
        /// \param newstate [in] The new state to be set for the thread 
        ///                 referenced by the \a id parameter.
        /// \param newstate_ex [in] The new extended state to be set for the 
        ///                 thread referenced by the \a id parameter.
        /// \param priority   [in] The priority with which the thread will be
        ///                   executed if the parameter \a newstate is pending.
        ///
        /// \returns        This function returns the previous state of the 
        ///                 thread referenced by the \a id parameter. It will 
        ///                 return one of the values as defined by the 
        ///                 \a thread_state enumeration. If the 
        ///                 thread is not known to the threadmanager the return 
        ///                 value will be \a thread_state#unknown.
        ///
        /// \note           If the thread referenced by the parameter \a id 
        ///                 is in \a thread_state#active state this function 
        ///                 schedules a new thread which will set the state of 
        ///                 the thread as soon as its not active anymore. The
        ///                 function returns \a thread_state#active in this case. 
        virtual thread_state set_state(thread_id_type id, 
            thread_state_enum newstate,
            thread_state_ex_enum newstate_ex = wait_signaled,
            thread_priority priority = thread_priority_normal,
            error_code& ec = throws) = 0;

        /// Set a timer to set the state of the given \a thread to the given 
        /// new value after it expired (at the given time)
        /// \brief  Set the thread state of the \a thread referenced by the 
        ///         thread_id \a id.
        ///
        /// Set a timer to set the state of the given \a thread to the given 
        /// new value after it expired (at the given time)
        ///
        /// \param id         [in] The thread id of the thread the state should 
        ///                   be modified for.
        /// \param at_time
        /// \param state      [in] The new state to be set for the thread 
        ///                   referenced by the \a id parameter.
        /// \param newstate_ex [in] The new extended state to be set for the 
        ///                   thread referenced by the \a id parameter.
        /// \param priority   [in] The priority with which the thread will be
        ///                   executed if the parameter \a newstate is pending.
        ///
        /// \returns
        virtual thread_id_type set_state (time_type const& expire_at, 
            thread_id_type id, thread_state_enum newstate = pending,
            thread_state_ex_enum newstate_ex = wait_timeout,
            thread_priority priority = thread_priority_normal,
            error_code& ec = throws) = 0;

        /// \brief  Set the thread state of the \a thread referenced by the 
        ///         thread_id \a id.
        ///
        /// Set a timer to set the state of the given \a thread to the given
        /// new value after it expired (after the given duration)
        /// 
        /// \param id         [in] The thread id of the thread the state should 
        ///                   be modified for.
        /// \param after_duration
        /// \param state      [in] The new state to be set for the thread 
        ///                   referenced by the \a id parameter.
        /// \param newstate_ex [in] The new extended state to be set for the 
        ///                   thread referenced by the \a id parameter.
        /// \param priority   [in] The priority with which the thread will be
        ///                   executed if the parameter \a newstate is pending.
        ///
        /// \returns
        virtual thread_id_type set_state (duration_type const& expire_from_now, 
            thread_id_type id, thread_state_enum newstate = pending,
            thread_state_ex_enum newstate_ex = wait_timeout,
            thread_priority priority = thread_priority_normal,
            error_code& ec = throws) = 0;

        /// The function get_thread_gid is part of the thread related API 
        /// allows to query the GID of one of the threads known to the 
        /// threadmanager.
        ///
        /// \param id         [in] The thread id of the thread the state should 
        ///                   be modified for.
        ///
        /// \returns          This function returns the GID of the 
        ///                   thread referenced by the \a id parameter. If the 
        ///                   thread is not known to the threadmanager the 
        ///                   return value will be \a naming::invalid_id.
        virtual naming::id_type const& get_thread_gid(thread_id_type id) = 0;

        /// The get_description function is part of the thread related API and 
        /// allows to query the description of one of the threads known to the 
        /// threadmanager
        ///
        /// \param id       [in] The thread id of the thread the description 
        ///                 should be returned for.
        ///
        /// \returns        This function returns the description of the 
        ///                 thread referenced by the \a id parameter. If the 
        ///                 thread is not known to the threadmanager the return 
        ///                 value will be the string "<unknown>".
        virtual std::string get_description(thread_id_type id) const = 0;
        virtual void set_description(thread_id_type id, char const* desc = 0) = 0;

        virtual std::string get_lco_description(thread_id_type id) const = 0;
        virtual void set_lco_description(thread_id_type id, char const* desc = 0) = 0;

        /// The function \a register_work adds a new work item to the thread 
        /// manager. It doesn't immediately create a new \a thread, it just adds 
        /// the task parameters (function, initial state and description) to 
        /// the internal management data structures. The thread itself will be 
        /// created when the number of existing threads drops below the number
        /// of threads specified by the constructors max_count parameter.
        ///
        /// \param func   [in] The function or function object to execute as 
        ///               the thread's function. This must have a signature as
        ///               defined by \a thread_function_type.
        /// \param description [in] The value of this parameter allows to 
        ///               specify a description of the thread to create. This 
        ///               information is used for logging purposes mainly, but 
        ///               might be useful for debugging as well. This parameter 
        ///               is optional and defaults to an empty string.
        /// \param initial_state
        ///               [in] The value of this parameter defines the initial 
        ///               state of the newly created \a thread. This must be
        ///               one of the values as defined by the \a thread_state 
        ///               enumeration (thread_state#pending, or \a
        ///               thread_state#suspended, any other value will throw a
        ///               hpx#bad_parameter exception).
        /// \param parent_prefix
        /// \param parent thread id
        virtual void
        register_work(thread_init_data& data, 
            thread_state_enum initial_state = pending,
            error_code& ec = throws) = 0;

        /// The function \a register_thread adds a new work item to the thread 
        /// manager. It creates a new \a thread, adds it to the internal
        /// management data structures, and schedules the new thread, if 
        /// appropriate.
        ///
        /// \param func   [in] The function or function object to execute as 
        ///               the thread's function. This must have a signature as
        ///               defined by \a thread_function_type.
        /// \param description [in] The value of this parameter allows to 
        ///               specify a description of the thread to create. This 
        ///               information is used for logging purposes mainly, but 
        ///               might be useful for debugging as well. This parameter 
        ///               is optional and defaults to an empty string.
        /// \param initial_state
        ///               [in] The value of this parameter defines the initial 
        ///               state of the newly created \a thread. This must be
        ///               one of the values as defined by the \a thread_state 
        ///               enumeration (thread_state#pending, or \a
        ///               thread_state#suspended, any other value will throw a
        ///               hpx#bad_parameter exception).
        /// \param run_now [in] If this parameter is \a true and the initial 
        ///               state is given as \a thread_state#pending the thread 
        ///               will be run immediately, otherwise it will be 
        ///               scheduled to run later (either this function is 
        ///               called for another thread using \a true for the
        ///               parameter \a run_now or the function \a 
        ///               threadmanager#do_some_work is called). This parameter
        ///               is optional and defaults to \a true.
        ///
        /// \returns      The function returns the thread id of the newly 
        ///               created thread. 
        virtual thread_id_type 
        register_thread(thread_init_data& data, 
            thread_state_enum initial_state = pending, 
            bool run_now = true, error_code& ec = throws) = 0;

        /// this notifies the thread manager that there is some more work 
        /// available 
        virtual void do_some_work(std::size_t num_thread = std::size_t(-1)) = 0;

        static std::size_t get_thread_num(bool* numa_sensitive = 0);

        void init_tss(std::size_t thread_num, bool numa_sensitive);
        void deinit_tss();

    private:
        // the TSS holds the number associated with a given OS thread
        static boost::thread_specific_ptr<std::size_t> thread_num_;
    };

    ///////////////////////////////////////////////////////////////////////////
    /// \class threadmanager threadmanager.hpp hpx/runtime/threads/threadmanager.hpp
    ///
    /// The \a threadmanager class is the central instance of management for
    /// all (non-depleted) \a thread's
    template <typename SchedulingPolicy, typename NotificationPolicy>
    class threadmanager_impl : public threadmanager_base
    {
    private:
        // we use a simple mutex to protect the data members of the 
        // threadmanager for now
        typedef boost::mutex mutex_type;

        // we use the boost::posix_time::ptime type for time representation
        typedef typename threadmanager_base::time_type time_type;

        // we use the boost::posix_time::time_duration type as the duration 
        // representation
        typedef typename threadmanager_base::duration_type duration_type;

    public:
        typedef SchedulingPolicy scheduling_policy_type;
        typedef NotificationPolicy notification_policy_type;


        ///
        threadmanager_impl(util::io_service_pool& timer_pool,
            scheduling_policy_type& scheduler,
            notification_policy_type& notifier);
        ~threadmanager_impl();

        /// The function \a register_work adds a new work item to the thread 
        /// manager. It doesn't immediately create a new \a thread, it just adds 
        /// the task parameters (function, initial state and description) to 
        /// the internal management data structures. The thread itself will be 
        /// created when the number of existing threads drops below the number
        /// of threads specified by the constructors max_count parameter.
        ///
        /// \param func   [in] The function or function object to execute as 
        ///               the thread's function. This must have a signature as
        ///               defined by \a thread_function_type.
        /// \param description [in] The value of this parameter allows to 
        ///               specify a description of the thread to create. This 
        ///               information is used for logging purposes mainly, but 
        ///               might be useful for debugging as well. This parameter 
        ///               is optional and defaults to an empty string.
        /// \param initial_state
        ///               [in] The value of this parameter defines the initial 
        ///               state of the newly created \a thread. This must be
        ///               one of the values as defined by the \a thread_state 
        ///               enumeration (thread_state#pending, or \a
        ///               thread_state#suspended, any other value will throw a
        ///               hpx#bad_parameter exception).
        void register_work(thread_init_data& data, 
            thread_state_enum initial_state = pending, 
            error_code& ec = throws);

        /// The function \a register_thread adds a new work item to the thread 
        /// manager. It creates a new \a thread, adds it to the internal
        /// management data structures, and schedules the new thread, if 
        /// appropriate.
        ///
        /// \param func   [in] The function or function object to execute as 
        ///               the thread's function. This must have a signature as
        ///               defined by \a thread_function_type.
        /// \param description [in] The value of this parameter allows to 
        ///               specify a description of the thread to create. This 
        ///               information is used for logging purposes mainly, but 
        ///               might be useful for debugging as well. This parameter 
        ///               is optional and defaults to an empty string.
        /// \param initial_state
        ///               [in] The value of this parameter defines the initial 
        ///               state of the newly created \a thread. This must be
        ///               one of the values as defined by the \a thread_state 
        ///               enumeration (thread_state#pending, or \a
        ///               thread_state#suspended, any other value will throw a
        ///               hpx#bad_parameter exception).
        /// \param run_now [in] If this parameter is \a true and the initial 
        ///               state is given as \a thread_state#pending the thread 
        ///               will be run immediately, otherwise it will be 
        ///               scheduled to run later (either this function is 
        ///               called for another thread using \a true for the
        ///               parameter \a run_now or the function \a 
        ///               threadmanager#do_some_work is called). This parameter
        ///               is optional and defaults to \a true.
        ///
        /// \returns      The function returns the thread id of the newly 
        ///               created thread. 
        thread_id_type register_thread(thread_init_data& data, 
            thread_state_enum initial_state = pending, 
            bool run_now = true, error_code& ec = throws);

        /// \brief  Run the thread manager's work queue. This function 
        ///         instantiates the specified number of OS threads. All OS
        ///         threads are started to execute the function \a tfunc.
        ///
        /// \param num_threads
        ///               [in] The initial number of threads to be started by
        ///               this thread manager instance. This parameter is 
        ///               optional and defaults to 1 (one).
        ///
        /// \returns      The function returns \a true if the thread manager
        ///               has been started successfully, otherwise it returns 
        ///               \a false.
        bool run(std::size_t num_threads = 1);

        /// \brief Forcefully stop the threadmanager
        ///
        /// \param blocking
        ///
        void stop (bool blocking = true);

        /// \brief Return whether the thread manager is still running
        state status() const
        {
            return state_.load();
        }

        /// \brief return the number of PX-threads with the given state
        ///
        /// \note This function lock the internal OS lock in the threadmanager
        boost::int64_t get_thread_count(thread_state_enum state = unknown) const;

        // \brief Abort all threads which are in suspended state. This will set 
        //        the state of all suspended threads to \a pending while 
        //        supplying the wait_abort extended state flag
        void abort_all_suspended_threads();

        // \brief Clean up terminated threads. This deletes all threads which 
        //        have been terminated but which are still held in the queue 
        //        of terminated threads. Some schedulers might not do anything
        //        here.
        bool cleanup_terminated();

        /// \brief Return the number of OS threads running in this threadmanager
        ///
        /// This function will return correct results only if the threadmanager 
        /// is running.
        std::size_t get_num_os_threads() const
        {
            mutex_type::scoped_lock lk(mtx_);
            return threads_.size();
        }

        /// The set_state function is part of the thread related API and allows
        /// to change the state of one of the threads managed by this 
        /// threadmanager.
        ///
        /// \param id       [in] The thread id of the thread the state should 
        ///                 be modified for.
        /// \param newstate [in] The new state to be set for the thread 
        ///                 referenced by the \a id parameter.
        /// \param newstate_ex [in] The new extended state to be set for the 
        ///                 thread referenced by the \a id parameter.
        /// \param priority   [in] The priority with which the thread will be
        ///                   executed if the parameter \a newstate is pending.
        ///
        /// \returns        This function returns the previous state of the 
        ///                 thread referenced by the \a id parameter. It will 
        ///                 return one of the values as defined by the 
        ///                 \a thread_state enumeration. If the 
        ///                 thread is not known to the threadmanager the return 
        ///                 value will be \a thread_state#unknown.
        ///
        /// \note           If the thread referenced by the parameter \a id 
        ///                 is in \a thread_state#active state this function 
        ///                 schedules a new thread which will set the state of 
        ///                 the thread as soon as its not active anymore. The
        ///                 function returns \a thread_state#active in this case. 
        thread_state set_state(thread_id_type id, thread_state_enum newstate,
            thread_state_ex_enum newstate_ex = wait_signaled,
            thread_priority priority = thread_priority_normal,
            error_code& ec = throws);

        /// The get_state function is part of the thread related API. It
        /// queries the state of one of the threads known to the threadmanager
        ///
        /// \param id       [in] The thread id of the thread the state should 
        ///                 be returned for.
        ///
        /// \returns        This function returns the current state of the 
        ///                 thread referenced by the \a id parameter. It will 
        ///                 return one of the values as defined by the 
        ///                 \a thread_state enumeration. If the 
        ///                 thread is not known to the threadmanager the return 
        ///                 value will be \a thread_state#unknown.
        thread_state get_state(thread_id_type id);
        
        /// The get_phase function is part of the thread related API. It 
        /// queries the phase of one of the threads known to the threadmanager
        ///
        /// \param id       [in] The thread id of the thread the phase should 
        ///                 be returned for.
        ///
        /// \returns        This function returns the current phase of the 
        ///                 thread referenced by the \a id parameter. If the
        ///                 thread is not known to the threadmanager the return 
        ///                 value will be ~0. 
        std::size_t get_phase(thread_id_type id);

        /// The function get_thread_gid is part of the thread related API 
        /// allows to query the GID of one of the threads known to the 
        /// threadmanager.
        ///
        /// \param id         [in] The thread id of the thread the state should 
        ///                   be modified for.
        ///
        /// \returns          This function returns the GID of the 
        ///                   thread referenced by the \a id parameter. If the 
        ///                   thread is not known to the threadmanager the 
        ///                   return value will be \a naming::invalid_id.
        naming::id_type const& get_thread_gid(thread_id_type id);

        /// Set a timer to set the state of the given \a thread to the given 
        /// new value after it expired (at the given time)
        /// \brief  Set the thread state of the \a thread referenced by the 
        ///         thread_id \a id.
        ///
        /// Set a timer to set the state of the given \a thread to the given 
        /// new value after it expired (at the given time)
        ///
        /// \param id         [in] The thread id of the thread the state should 
        ///                   be modified for.
        /// \param at_time
        /// \param state      [in] The new state to be set for the thread 
        ///                   referenced by the \a id parameter.
        /// \param newstate_ex [in] The new extended state to be set for the 
        ///                   thread referenced by the \a id parameter.
        /// \param priority   [in] The priority with which the thread will be
        ///                   executed if the parameter \a new_state is pending.
        ///
        /// \returns
        thread_id_type set_state (time_type const& expire_at, 
            thread_id_type id, thread_state_enum newstate = pending,
            thread_state_ex_enum newstate_ex = wait_timeout,
            thread_priority priority = thread_priority_normal,
            error_code& ec = throws);

        /// \brief  Set the thread state of the \a thread referenced by the 
        ///         thread_id \a id.
        ///
        /// Set a timer to set the state of the given \a thread to the given
        /// new value after it expired (after the given duration)
        /// 
        /// \param id         [in] The thread id of the thread the state should 
        ///                   be modified for.
        /// \param after_duration
        /// \param state      [in] The new state to be set for the thread 
        ///                   referenced by the \a id parameter.
        /// \param newstate_ex [in] The new extended state to be set for the 
        ///                   thread referenced by the \a id parameter.
        /// \param priority   [in] The priority with which the thread will be
        ///                   executed if the parameter \a new_state is pending.
        ///
        /// \returns
        thread_id_type set_state (duration_type const& expire_from_now, 
            thread_id_type id, thread_state_enum newstate = pending,
            thread_state_ex_enum newstate_ex = wait_timeout,
            thread_priority priority = thread_priority_normal,
            error_code& ec = throws);

        /// The get_description function is part of the thread related API and 
        /// allows to query the description of one of the threads known to the 
        /// threadmanager
        ///
        /// \param id       [in] The thread id of the thread the description 
        ///                 should be returned for.
        ///
        /// \returns        This function returns the description of the 
        ///                 thread referenced by the \a id parameter. If the 
        ///                 thread is not known to the thread-manager the return 
        ///                 value will be the string "<unknown>".
        std::string get_description(thread_id_type id) const;
        void set_description(thread_id_type id, char const* desc = 0);

        std::string get_lco_description(thread_id_type id) const;
        void set_lco_description(thread_id_type id, char const* desc = 0);

        /// Get percent maintenance time in main thread-manager loop.
        boost::int64_t avg_maint_ratio() const;
        boost::int64_t avg_maint_ratio(std::size_t num_thread) const;

    protected:
        // this is the thread function executing the work items in the queue
        void tfunc(std::size_t num_thread, std::size_t& theads_run);
        std::size_t tfunc_impl(std::size_t num_thread);

        // thread function registered for set_state if thread is currently 
        // active
        thread_state set_active_state(thread_id_type id, 
                thread_state_enum newstate, thread_state_ex_enum newstate_ex,
                thread_priority priority);

    public:
        /// this notifies the thread manager that there is some more work 
        /// available 
        void do_some_work(std::size_t num_thread = std::size_t(-1))
        {
            scheduler_.do_some_work(num_thread);
        }

        /// API functions forwarding to notification policy
        void report_error(std::size_t num_thread, boost::exception_ptr const& e)
        {
            notifier_.on_error(num_thread, e);
            scheduler_.on_error(num_thread, e);
        }

        std::size_t get_executed_threads(std::size_t num = std::size_t(-1)) const;

    protected:
        /// This thread function is used by the at_timer thread below to trigger
        /// the required action.
        thread_state_enum wake_timer_thread (thread_id_type id, 
            thread_state_enum newstate, thread_state_ex_enum newstate_ex, 
            thread_priority priority, thread_id_type timer_id, 
            boost::shared_ptr<boost::atomic<bool> > triggered);

        /// This thread function initiates the required set_state action (on 
        /// behalf of one of the threadmanager#set_state functions).
        template <typename TimeType>
        thread_state_enum at_timer (TimeType const& expire, thread_id_type id, 
            thread_state_enum newstate, thread_state_ex_enum newstate_ex,
            thread_priority priority);

#if HPX_AGAS_VERSION > 0x10
    public:
        /// install_counters is called during startup to allow registration of 
        /// performance counters 
        void install_counters();
#endif

    private:
        /// this thread manager has exactly as much threads as requested
        mutable mutex_type mtx_;                    ///< mutex protecting the members
        boost::barrier* startup_;                   ///< startup synchronization

        boost::ptr_vector<boost::thread> threads_;
        std::vector<std::size_t> executed_threads_;
        boost::atomic<long> thread_count_;

        atomic_state state_;                ///< thread manager state 
        util::io_service_pool& timer_pool_; ///< used for timed set_state

        util::block_profiler<register_thread_tag> thread_logger_;
        util::block_profiler<register_work_tag> work_logger_;
        util::block_profiler<set_state_tag> set_state_logger_;

        scheduling_policy_type& scheduler_;
        notification_policy_type& notifier_;

        // tfunc_impl timers
        std::vector<boost::uint64_t> exec_times, tfunc_times;
    };
}}

#include <hpx/config/warnings_suffix.hpp>

#endif 
