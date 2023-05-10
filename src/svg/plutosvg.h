#ifndef __PLUTOSVG_H__
#define __PLUTOSVG_H__

#include "plutovg.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Load the image from a file in memory
 * @param data - pointer to the file data in memory
 * @param size - size of the data to load, in bytes
 * @param font - font to use for text rendering
 * @param width - requested width, in pixels
 * @param height - requested height, in pixels
 * @param dpi - dots per inch to use for units conversion to pixels
 * @return pointer to surface object on success, otherwise NULL
 */
plutovg_surface_t* plutosvg_load_from_memory(const char* data, int size, plutovg_font_t* font, int width, int height, double dpi);

#if 0
/**
 * @brief Load the image from a file on disk
 * @param filename - path of the image file to load
 * @param font - font to use for text rendering
 * @param width - requested width, in pixels
 * @param height - requested height, in pixels
 * @param dpi - dots per inch to use for units conversion to pixels
 * @return pointer to surface object on success, otherwise NULL
 */
plutovg_surface_t* plutosvg_load_from_file(const char* filename, plutovg_font_t* font, int width, int height, double dpi);
#endif 

/**
 * @brief Get image dimensions from a file in memory
 * @param data - pointer to the file data in memory
 * @param size - size of the data to load, in bytes
 * @param font - font to use for text rendering
 * @param width - width of the image, in pixels
 * @param height - height of the image, in pixels
 * @param dpi - dots per inch to use for units conversion to pixels
 * @return true on success, otherwise false
 */
int plutosvg_dimensions_from_memory(const char* data, int size, plutovg_font_t* font, int* width, int* height, double dpi);

#if 0
/**
 * @brief Get image dimensions from a file on disk
 * @param filename - path of the image file to load
 * @param font - font to use for text rendering
 * @param width - width of the image, in pixels
 * @param height - height of the image, in pixels
 * @param dpi - dots per inch to use for units conversion to pixels
 * @return true on success, otherwise false
 */
int plutosvg_dimensions_from_file(const char* filename, plutovg_font_t* font, int* width, int* height, double dpi);
#endif 

#ifdef __cplusplus
}
#endif

#endif /* __PLUTOSVG_H__ */
