// jcamsys_camerasetting.h
// sent by the server to the camera upon registration
// 1 May 2018
//
//

#include <stdint.h>


// Use 16 or 32 bit types, not all machines we be 64 bit and we want a reasonable
// chance of the data type being atomic.
struct __attribute__((packed)) jcamsys_statistics
{
	uint32_t	archive_files_written;
	uint32_t	archive_file_write_errors;

	// note these are periodically updated, not real-time
	uint32_t	archive_numfiles;		// number of images in archive
	uint32_t	archive_oldest;			// c time (seconds)


	// These values enable viewers to select window size and image dimensions
	uint16_t	camera_largest_width_active;
	uint16_t	camera_largest_height_active;
	uint16_t	camera_largest_width_configured;
	uint16_t	camera_largest_height_configured;
};



// Prototypes



