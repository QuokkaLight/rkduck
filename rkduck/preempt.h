#ifndef _PREEMPT_H_
#define _PREEMPT_H_

// #ifdef CONFIG_PREEMPT_COUNT

#define preempt_enable_no_resched() \
do { \
	barrier(); \
	preempt_count_dec(); \
} while (0)

/* #ifdef CONFIG_PREEMPT

// #define preempt_check_resched() \
// do { \
// 	if (should_resched(0)) \
// 		__preempt_schedule(); \
// } while (0)

#define preempt_check_resched() do { } while (0)
#endif  CONFIG_PREEMPT 
*/

#define preempt_enable_no_resched_notrace() \
do { \
	barrier(); \
	__preempt_count_dec(); \
} while (0)

// #endif /* CONFIG_PREEMPT_COUNT */

#endif /* _PREEMPT_H_ */