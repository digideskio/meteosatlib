#ifndef IMAGE_DATA_H
#define IMAGE_DATA_H

#include <string>

namespace msat {

struct ImageData;

struct Image {
  /// Image name
  std::string name;

  /// Image time
  int year, month, day, hour, minute;

	/// Longitude of sub-satellite point
	float sublon;

  int channel_id;
  int spacecraft_id;

	// TODO
  int column_factor;
	// TODO
  int line_factor;

	// Horizontal position of the image on the entire world view
  int column_offset;

	// Vertical position of the image on the entire world view
  int line_offset;

	/// Pixel resolution at nadir point
	float pixelSize() const;

	/// Earth dimension scanned by Seviri in the X direction
	float seviriDX() const;

	/// Earth dimension scanned by Seviri in the Y direction
	float seviriDY() const;

  // Get the datetime as a string
  std::string datetime() const;

  // Get the image time as number of seconds since 1/1/2000 UTC
  time_t forecastSeconds2000() const;

	// Image data
	ImageData* data;

	Image() : data(0) {}
	~Image();

	/// Set the image data for this image
	void setData(ImageData* data);

	/**
	 * Crop the image to the given rectangle specified in coordinates relative
	 * to the image itself
	 */
	void crop(int x, int y, int width, int height);
};

/// Interface for image data of various types
struct ImageData
{
	ImageData() : columns(0), lines(0), slope(1), offset(0), bpp(0) {}
  virtual ~ImageData() {}

  // Image metadata

  /// Number of columns
  size_t columns;

  /// Number of lines
  size_t lines;

  /// Scaling factor to apply to the raw image data to get the real physical
  /// values
  float slope;

  /// Reference value to add to the scaled raw image data to get the real
  /// physical values
  float offset;

  /// Number of bits per sample
  size_t bpp;

  /// Image sample as physical value (already scaled with slope and offset)
  virtual float scaled(int column, int line) const
  {
    return unscaled(column, line) * slope + offset;
  }

	/// Get all the lines * columns samples, scaled
	virtual float* allScaled() const;

  /// Image sample as unscaled int value (to be scaled with slope and offset)
  virtual int unscaled(int column, int line) const = 0;

	/// Get all the lines * columns samples, unscaled
	virtual int* allUnscaled() const;

	/// Return the decimal scaling factor that can be used before truncating
	/// scaled values as integers
	int decimalScale() const;

	/// Crop the image to the given rectangle
	virtual void crop(int x, int y, int width, int height) = 0;
};

// Container for image data, which can be used with different sample sizes
template<typename EL>
struct ImageDataWithPixels : public ImageData
{
public:
  typedef EL Sample;
  Sample* pixels;

  ImageDataWithPixels() : pixels(0) { bpp = sizeof(Sample) * 8; }
  ImageDataWithPixels(size_t width, size_t height) : pixels(new Sample[width*height])
	{
		bpp = sizeof(Sample) * 8;
		columns = width;
		lines = height;
	}
  ~ImageDataWithPixels()
  {
    if (pixels)
      delete[] pixels;
  }

  virtual int unscaled(int column, int line) const
  {
      return static_cast<int>(pixels[line * columns + column]);
  }

	// Rotate the image by 180 degrees, in place
	void rotate180()
	{
		for (int y = 0; y < lines; ++y)
			for (int x = 0; x < columns; ++x)
			{
				Sample i = pixels[y * columns + x];
				pixels[y * columns + x] = pixels[(lines - y - 1) * columns + (columns - x - 1)];
				pixels[(lines - y - 1) * columns + (columns - x - 1)] = i;
			}
	}

	// Throw away all the samples outside of a given area
	void crop(int x, int y, int width, int height);
};

struct ImageConsumer
{
	virtual ~ImageConsumer() {}
	virtual void processImage(const Image& image) = 0;
};

struct ImageImporter
{
	int cropX, cropY, cropWidth, cropHeight;

	ImageImporter() : cropX(0), cropY(0), cropWidth(0), cropHeight(0) {}
	virtual ~ImageImporter() {}

	bool shouldCrop() const { return cropWidth != 0 && cropHeight != 0; }

	virtual void read(ImageConsumer& output) = 0;
};

std::auto_ptr<ImageConsumer> createImageDumper(bool withContents);

}

// vim:set ts=2 sw=2:
#endif
