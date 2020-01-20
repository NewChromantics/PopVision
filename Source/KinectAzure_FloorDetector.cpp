//#include "SampleMathTypes.h"
#include <k4a/k4a.h>
#include <k4a/k4atypes.h>
#include <vector>
#include <optional>
//#include "PointCloudGenerator.h"
//#include "Utilities.h"
#include <cmath>
//#include "FloorDetector.h"
#include <algorithm>    // std::sort
#include <cassert>      // assert
#include <numeric>      // std::iota


namespace Samples
{
	struct Vector
	{
		float X;
		float Y;
		float Z;

		Vector(float x, float y, float z)
			: X(x)
			, Y(y)
			, Z(z)
		{
		}

		Vector(const k4a_float3_t& v)
			: X(v.xyz.x)
			, Y(v.xyz.y)
			, Z(v.xyz.z)
		{
		}

		float Dot(const Vector& other) const
		{
			return X * other.X + Y * other.Y + Z * other.Z;
		}

		float SquareLength() const
		{
			return X * X + Y * Y + Z * Z;
		}

		float Length() const
		{
			return std::sqrt(SquareLength());
		}

		Vector operator*(float c) const
		{
			return { X * c, Y * c, Z * c };
		}

		Vector operator/(float c) const
		{
			return *this * (1 / c);
		}

		Vector Normalized() const
		{
			return *this / Length();
		}

		float Angle(const Vector& other) const
		{
			return std::acos(Dot(other) / Length() / other.Length());
		}
	};

	inline Vector operator-(const Vector& v1, const Vector& v2)
	{
		return { v1.X - v2.X, v1.Y - v2.Y, v1.Z - v2.Z };
	}

	inline Vector operator+(const Vector& v1, const Vector& v2)
	{
		return { v1.X + v2.X, v1.Y + v2.Y, v1.Z + v2.Z };
	}

	inline Vector operator*(float c, const Vector& v)
	{
		return { v.X * c, v.Y * c, v.Z * c };
	}

	inline Vector operator*(const Vector& v1, const Vector& v2)
	{
		return { v1.Y * v2.Z - v1.Z * v2.Y, v1.Z * v2.X - v1.X * v2.Z, v1.X * v2.Y - v1.Y * v2.X };
	}

	struct Plane
	{
		using Point = Vector;

		Vector Normal;
		Point Origin;
		float C;

		static Plane Create(Vector n, Point p)
		{
			float c = n.X * p.X + n.Y * p.Y + n.Z * p.Z;
			return { n, p, -c };
		}

		static Plane Create(const Point& p1, const Point& p2, const Point& p3)
		{
			Vector v1 = p2 - p1;
			Vector v2 = p2 - p3;
			Vector n = v1 * v2;
			return Create(n, p1);
		}

		Vector ProjectVector(const Vector& v) const
		{
			return v - Normal * (v.Dot(Normal) / Normal.SquareLength());
		}

		Vector ProjectPoint(const Point& p) const
		{
			return Origin + ProjectVector(p - Origin);
		}

		float AbsDistance(const Point& p) const
		{
			return std::abs(p.X * Normal.X + p.Y * Normal.Y + p.Z * Normal.Z + C) / Normal.Length();
		}
	};
}

namespace Samples
{
	class PointCloudGenerator
	{
	public:
		PointCloudGenerator(const k4a_calibration_t& sensorCalibration);
		~PointCloudGenerator();

		void Update(k4a_image_t depthImage);
		const std::vector<k4a_float3_t>& GetCloudPoints(int downsampleStep = 1);

	private:
		k4a_transformation_t m_transformationHandle = nullptr;
		k4a_image_t m_pointCloudImage_int16x3 = nullptr;
		std::vector<k4a_float3_t> m_cloudPoints;
	};
}
namespace Samples
{
	std::optional<Samples::Vector> TryEstimateGravityVectorForDepthCamera(
		const k4a_imu_sample_t& imuSample,
		const k4a_calibration_t& sensorCalibration);

	class FloorDetector
	{
	public:
		static std::optional<Samples::Plane> TryDetectFloorPlane(
			const std::vector<k4a_float3_t>& cloudPoints,
			const k4a_imu_sample_t& imuSample,
			const k4a_calibration_t& sensorCalibration,
			size_t minimumFloorPointCount);
	};
}

std::optional<Samples::Vector> Samples::TryEstimateGravityVectorForDepthCamera(
	const k4a_imu_sample_t& imuSample,
	const k4a_calibration_t& sensorCalibration)
{
	// An accelerometer at rest on the surface of the Earth will measure an acceleration
	// due to Earth's gravity straight **upwards** (by definition) of g ~ 9.81 m/s2.
	// https://en.wikipedia.org/wiki/Accelerometer

	Samples::Vector imuAcc = imuSample.acc_sample; // in meters per second squared.

	// Estimate gravity when device is not moving.
	if (std::abs(imuAcc.Length() - 9.81f) < 0.2f)
	{
		// Extrinsic Rotation from ACCEL to DEPTH.
		const auto& R = sensorCalibration.extrinsics[K4A_CALIBRATION_TYPE_ACCEL][K4A_CALIBRATION_TYPE_DEPTH].rotation;

		Samples::Vector Rx = { R[0], R[1], R[2] };
		Samples::Vector Ry = { R[3], R[4], R[5] };
		Samples::Vector Rz = { R[6], R[7], R[8] };
		Samples::Vector depthAcc = { Rx.Dot(imuAcc), Ry.Dot(imuAcc) , Rz.Dot(imuAcc) };

		// The acceleration due to gravity, g, is in a direction toward the ground.
		// However an accelerometer at rest in a gravity field reports upward acceleration
		// relative to the local inertial frame (the frame of a freely falling object).
		Samples::Vector depthGravity = depthAcc * -1;
		return depthGravity;
	}
	return {};
}

struct HistogramBin
{
	size_t count;
	float leftEdge;
};

std::vector<HistogramBin> Histogram(const std::vector<float>& values, float binSize)
{
	// Bounds
	const auto[minEl, maxEl] = std::minmax_element(values.begin(), values.end());
	const float minVal = *minEl;
	const float maxVal = *maxEl;

	size_t binCount = 1 + static_cast<size_t>((maxVal - minVal) / binSize);

	// Bins
	std::vector<HistogramBin> histBins{ binCount };
	for (size_t i = 0; i < binCount; ++i)
	{
		histBins[i] = { /*binCount*/ 0, /*binLeftEdge*/ i * binSize + minVal };
	}

	// Counts
	for (float v : values)
	{
		int binIndex = static_cast<int>((v - minVal) / binSize);
		histBins[binIndex].count++;
	}
	return histBins;
}

std::optional<Samples::Plane> FitPlaneToInlierPoints(const std::vector<k4a_float3_t>& cloudPoints, const std::vector<size_t>& inlierIndices)
{
	// https://www.ilikebigbits.com/2015_03_04_plane_from_points.html

	if (inlierIndices.size() < 3)
	{
		return {};
	}

	// Compute centroid.
	Samples::Vector centroid(0, 0, 0);
	for (const auto& index : inlierIndices)
	{
		centroid = centroid + cloudPoints[index];
	}
	centroid = centroid / static_cast<float>(inlierIndices.size());

	// Compute the zero-mean 3x3 symmetric covariance matrix relative.
	float xx = 0, xy = 0, xz = 0, yy = 0, yz = 0, zz = 0;
	for (const auto& index : inlierIndices)
	{
		Samples::Vector r = cloudPoints[index] - centroid;
		xx += r.X * r.X;
		xy += r.X * r.Y;
		xz += r.X * r.Z;
		yy += r.Y * r.Y;
		yz += r.Y * r.Z;
		zz += r.Z * r.Z;
	}

	float detX = yy * zz - yz * yz;
	float detY = xx * zz - xz * xz;
	float detZ = xx * yy - xy * xy;

	float detMax = std::max({ detX, detY, detZ });
	if (detMax <= 0)
	{
		return {};
	}

	Samples::Vector normal(0, 0, 0);
	if (detMax == detX)
	{
		normal = { detX, xz * yz - xy * zz, xy * yz - xz * yy };
	}
	else if (detMax == detY)
	{
		normal = { xz * yz - xy * zz, detY, xy * xz - yz * xx };
	}
	else
	{
		normal = { xy * yz - xz * yy, xy * xz - yz * xx, detZ };
	}

	return Samples::Plane::Create(normal.Normalized(), centroid);
}

std::optional<Samples::Plane> Samples::FloorDetector::TryDetectFloorPlane(
	const std::vector<k4a_float3_t>& cloudPoints,
	const k4a_imu_sample_t& imuSample,
	const k4a_calibration_t& sensorCalibration,
	size_t minimumFloorPointCount)
{
	auto gravity = TryEstimateGravityVectorForDepthCamera(imuSample, sensorCalibration);
	if (gravity.has_value() && !cloudPoints.empty())
	{
		// Up normal is opposite to gravity down vector.
		Samples::Vector up = (gravity.value() * -1).Normalized();

		// Compute elevation of cloud points (projections on the floor normal).
		std::vector<float> offsets(cloudPoints.size());
		for (size_t i = 0; i < cloudPoints.size(); ++i)
		{
			offsets[i] = up.Dot(Samples::Vector(cloudPoints[i]));
		}

		// There could be several horizontal planes in the scene (floor, tables, ceiling).
		// For the floor, look for lowest N points whose elevations are within a small range from each other.

		const float planeDisplacementRangeInMeters = 0.050f; // 5 cm in meters.
		const float planeMaxTiltInDeg = 5.0f;

		const int binAggregation = 6;
		assert(binAggregation >= 1);
		const float binSize = planeDisplacementRangeInMeters / binAggregation;
		const auto histBins = Histogram(offsets, binSize);

		// Cumulative histogram counts.
		std::vector<size_t> cumulativeHistCounts(histBins.size());
		cumulativeHistCounts[0] = histBins[0].count;
		for (size_t i = 1; i < histBins.size(); ++i)
		{
			cumulativeHistCounts[i] = cumulativeHistCounts[i - 1] + histBins[i].count;
		}

		assert(cumulativeHistCounts.back() == offsets.size());

		for (size_t i = 1; i + binAggregation < cumulativeHistCounts.size(); ++i)
		{
			size_t aggBinStart = i;                 // inclusive bin
			size_t aggBinEnd = i + binAggregation;  // exclusive bin
			size_t inlierCount = cumulativeHistCounts[aggBinEnd - 1] - cumulativeHistCounts[aggBinStart - 1];
			if (inlierCount > minimumFloorPointCount)
			{
				float offsetStart = histBins[aggBinStart].leftEdge; // inclusive
				float offsetEnd = histBins[aggBinEnd].leftEdge;     // exclusive

				// Inlier indices.
				std::vector<size_t> inlierIndices;
				for (size_t j = 0; j < offsets.size(); ++j)
				{
					if (offsetStart <= offsets[j] && offsets[j] < offsetEnd)
					{
						inlierIndices.push_back(j);
					}
				}

				// Fit plane to inlier points.
				auto refinedPlane = FitPlaneToInlierPoints(cloudPoints, inlierIndices);

				if (refinedPlane.has_value())
				{
					// Ensure normal is upward.
					if (refinedPlane->Normal.Dot(up) < 0)
					{
						refinedPlane->Normal = refinedPlane->Normal * -1;
					}

					// Ensure normal is mostly vertical.
					auto floorTiltInDeg = acos(refinedPlane->Normal.Dot(up)) * 180.0f / 3.14159265f;
					if (floorTiltInDeg < planeMaxTiltInDeg)
					{
						// For reduced jitter, use gravity for floor normal.
						refinedPlane->Normal = up;
						return refinedPlane;
					}
				}

				return {};
			}
		}
	}

	return {};
}


// K4A SDK is currently missing a point cloud pixel type returned
// by k4a_transformation_depth_image_to_point_cloud().
typedef union
{
	/** XYZ or array representation of vector. */
	struct _xyz
	{
		int16_t x; /**< X component of a vector. */
		int16_t y; /**< Y component of a vector. */
		int16_t z; /**< Z component of a vector. */
	} xyz;         /**< X, Y, Z representation of a vector. */
	int16_t v[3];    /**< Array representation of a vector. */
} PointCloudPixel_int16x3_t;


Samples::PointCloudGenerator::PointCloudGenerator(const k4a_calibration_t& sensorCalibration)
{
	int depthWidth = sensorCalibration.depth_camera_calibration.resolution_width;
	int depthHeight = sensorCalibration.depth_camera_calibration.resolution_height;

	// Create transformation handle
	m_transformationHandle = k4a_transformation_create(&sensorCalibration);

	auto Result = k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
		depthWidth,
		depthHeight,
		depthWidth * (int)sizeof(PointCloudPixel_int16x3_t),
		&m_pointCloudImage_int16x3);
	Kinect::IsOkay(Result, "k4a_image_create Create Point Cloud Image failed");
}

Samples::PointCloudGenerator::~PointCloudGenerator()
{
	if (m_transformationHandle != nullptr)
	{
		k4a_transformation_destroy(m_transformationHandle);
		m_transformationHandle = nullptr;
	}

	if (m_pointCloudImage_int16x3 != nullptr)
	{
		k4a_image_release(m_pointCloudImage_int16x3);
		m_pointCloudImage_int16x3 = nullptr;
	}
}

void Samples::PointCloudGenerator::Update(k4a_image_t depthImage)
{
	auto Result = k4a_transformation_depth_image_to_point_cloud(
		m_transformationHandle,
		depthImage,
		K4A_CALIBRATION_TYPE_DEPTH,
		m_pointCloudImage_int16x3);
	Kinect::IsOkay( Result, "Transform depth image to point clouds failed");
}

const std::vector<k4a_float3_t>& Samples::PointCloudGenerator::GetCloudPoints(int step)
{
	int width = k4a_image_get_width_pixels(m_pointCloudImage_int16x3);
	int height = k4a_image_get_height_pixels(m_pointCloudImage_int16x3);

	// Current SDK transforms a depth map to point cloud only as int16 type.
	// The point cloud conversion below to float is relatively slow.
	// It would be better for the SDK to provide this functionality directly.

	const auto pointCloudImageBufferInMM = (PointCloudPixel_int16x3_t*)k4a_image_get_buffer(m_pointCloudImage_int16x3);

	m_cloudPoints.resize(width * height / (step * step));
	size_t cloudPointsIndex = 0;
	for (int h = 0; h < height; h += step)
	{
		for (int w = 0; w < width; w += step)
		{
			int pixelIndex = h * width + w;

			// When the point cloud is invalid, the z-depth value is 0.
			if (pointCloudImageBufferInMM[pixelIndex].xyz.z > 0)
			{
				const float MillimeterToMeter = 0.001f;
				k4a_float3_t positionInMeter = {
					static_cast<float>(pointCloudImageBufferInMM[pixelIndex].v[0])* MillimeterToMeter,
					static_cast<float>(pointCloudImageBufferInMM[pixelIndex].v[1])* MillimeterToMeter,
					static_cast<float>(pointCloudImageBufferInMM[pixelIndex].v[2])* MillimeterToMeter };

				m_cloudPoints[cloudPointsIndex++] = positionInMeter;
			}
		}
	}
	m_cloudPoints.resize(cloudPointsIndex);

	return m_cloudPoints;
}

