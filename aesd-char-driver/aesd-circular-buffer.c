/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
//#include <stdio.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer. 
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
			size_t char_offset, size_t *entry_offset_byte_rtn )
{
	/**
	* TODO: implement per description
	*/
	uint8_t index;

	struct aesd_buffer_entry *entry;

	// Because the buffer starts from 0 we actually need the (char_offset+1)th element
	char_offset++;
	
	// Start at the out pointer
	index = buffer->out_offs;
	for (int count = 0; count < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; count++)
	{
		index = (buffer->out_offs + count) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

		// String at current index
		entry = &((buffer)->entry[index]);
		if(entry == NULL)
		{
			return NULL;
		}
		else
		{
			if(char_offset <= entry->size)
			{
				// offset is present in this string
				*entry_offset_byte_rtn = char_offset -1;
				return entry;
			}
			else
			{
				// Update char_offset and move to the next iteration
				char_offset -= entry->size;
			}
		}
	}

	// The char_offset doesn't exist
	return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
* 
* Assignment 8
* TODO: Since memory needs to be managed by the caller, in case of an overwrite, we need to return the pointer of the 
* 		overwritten buffer so that the caller can free it
*
* @return :	NULL 	- if no entry was overwritten (ie buffer was not full)
*			char*	- pointer to the buffer which was overwritten (ie buffer was full)
* 
* @note :	See lecture video for a possible implementation
*			Will need to modify the write_circular_buffer_packet function in to test this functionality
			Location - ../assignment-autotest/test/assignment7/Test_circular_buffer.c
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
	// could be used by multiple drivers hence cannot free here
    
	// No matter the status of the buffer (full or not) we have to overwrite at the position in_offs
	buffer->entry[buffer->in_offs] = *add_entry;

   	// Increment the in_offs and wrap around if reached the end
	buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

	// Only if the buffer is full, increment the in_offs and wrap around if reached the end 
	if(buffer->full ==  true)
	{
		buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
	}

   	// Check if the buffer is full after the above update
	if(buffer->in_offs == buffer->out_offs)
	{
		buffer->full = true;
	}
	else
	{
		buffer->full = false;
	}

}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
