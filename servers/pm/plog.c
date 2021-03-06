#include "pm.h"
#include "mproc.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// The structure of the time logs
typedef struct plog
{
	int p_id;
	time_t start_t;
	time_t end_t;
} plog;

// The circular buffer used to store info
typedef struct circularBuffer
{
	int cur_index;
	size_t size;
	plog* arr[PLOG_BUFFER_SIZE];
} circularBuffer;

circularBuffer buffer;

// Defines whether or not the buffer has been initialized to 0
bool dirtyBuf = true;
// Flips when started or stopped by the user, defines when to put new processes into buffer
bool started = false;

/* Entry point into functionality */
int do_plog()
{
	// Switch statement that uses message passed by library to define what function to run
	switch (m_in.m1_i1) {
	case PLOG_START:
		return plog_start();
	case PLOG_STOP:
		return plog_stop();
	case PLOG_RESETBUF:
		return plog_clear();
	case PLOG_GETBYIDX:
		return plog_IDXget();
	case PLOG_GETBYPID:
		return plog_PIDget();
	case PLOG_GETSIZE:
		return plog_get_size();
	default:
		return EXIT_FAILURE;
	}
}

/* Starts process logger process */
int plog_start()
{
	if (!started)
	{
		if (dirtyBuf)
			init_buffer();
		started = true;
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

/* Stops process logger process */
int plog_stop()
{
	if (!started)
		return (EXIT_FAILURE);

	started = false;
	return EXIT_SUCCESS;
}

/* Adds a new process to the buffer */
int log_start(int id)
{
	if (started)
	{
		// Gets the pointer to the next array element
		plog* tmp = buffer.arr[buffer.cur_index];
		// If pointer is null allocate memory for it
		if (!tmp)
		{
			tmp = (plog*)malloc(sizeof(plog));
			buffer.size = buffer.size + 1;
		}
		// Place info into memory
		tmp->p_id = id;
		do_time();
		tmp->start_t = mp->mp_reply.m2_l1;
		tmp->end_t = -1;
		buffer.arr[buffer.cur_index++] = tmp;

		if (buffer.cur_index == PLOG_BUFFER_SIZE)
			buffer.cur_index = 0;
		return (EXIT_SUCCESS);
	}
	return EXIT_FAILURE;
}

/* Adds termination time */
int log_end(int id)
{
	if (started)
	{
		// First finds the plog entry, to avoid writing to a process that no longer exists
		plog* found = find_by_PID(id);
		if (found)
		{
			do_time();
			found->end_t = mp->mp_reply.m2_l1;
			return EXIT_SUCCESS;
		}
	}

	return EXIT_FAILURE;
}

/* Clears entire buffer */
int plog_clear()
{
	if (started)
	{
		/* For each value in the array we want to free the memory */
		for (int i = 0; i < buffer.size; i++)
		{
			/* Sanity check for null pointers (may be unneccesary) */
			if (buffer.arr[i]) { free(buffer.arr[i]); }
		}
	}
	buffer.size = 0;
	buffer.cur_index = 0;
	return EXIT_SUCCESS;
}

/* Get current size of buffer */
int plog_get_size()
{
	// Set reply to the size of the buffer
	mp->mp_reply.m2_i1 = buffer.size;
	return EXIT_SUCCESS;
}

/* Get process by PID */
int plog_PIDget()
{
	plog* found = find_by_PID(m_in.m1_i2);
	if (found)
	{
		mp->mp_reply.m2_l1 = found->start_t;
		mp->mp_reply.m2_l2 = found->end_t;
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

/* Get process by index */
int plog_IDXget()
{
	// if the index given is within the size constraints returns plog info at index
	if (buffer.size > m_in.m1_i3 && m_in.m1_i3 >= 0)
	{
		const plog* tmp = buffer.arr[m_in.m1_i3];
		mp->mp_reply.m2_l1 = tmp->start_t;
		mp->mp_reply.m2_l2 = tmp->end_t;
		mp->mp_reply.m2_i1 = tmp->p_id;
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

// Basic linear search through buffer to find the desired PID
plog* find_by_PID(int id)
{
	for (int i = 0; i < buffer.size; i++)
	{
		if (buffer.arr[i])
		{
			if (id == buffer.arr[i]->p_id)
			{
				return buffer.arr[i];
			}
		}
	}
	return NULL;
}

// Initializes buffer with null pointers
void init_buffer()
{
	for (int i = 0; i < PLOG_BUFFER_SIZE; i++)
	{
		buffer.arr[i] = NULL;
	}
	buffer.cur_index = 0;
	buffer.size = 0;
	dirtyBuf = false;
}