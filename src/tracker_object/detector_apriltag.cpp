#include "agimus_vision/tracker_object/detector_apriltag.hpp"

#include <visp3/core/vpColor.h>
#include <visp3/core/vpDisplay.h>
#include <visp3/core/vpPoint.h>
#include <visp3/core/vpPixelMeterConversion.h>
#include <visp3/vision/vpPose.h>

namespace agimus_vision {
namespace tracker_object {

// Init common tag detector
vpDetectorAprilTag DetectorAprilTag::Apriltag_detector{};
vpImage<float> DetectorAprilTag::_depth{};


DetectorAprilTag::DetectorAprilTag( const vpCameraParameters &cam_parameters, const int tag_id, const double tag_size_meters )
  : Detector( cam_parameters )
  , _tag_id{ tag_id }
  , _tag_size_meters{ tag_size_meters }
{}

bool DetectorAprilTag::analyseImage( const vpImage< unsigned char > &gray_image, const vpImage< float > &depth )
{
    _depth = depth;
    return Apriltag_detector.detect( gray_image );
}

bool DetectorAprilTag::detect()
{
    _image_points.clear();

    for( size_t i{ 0 } ; i < Apriltag_detector.getNbObjects() ; ++i )
    {
        std::string tag = " " + std::to_string( _tag_id );
        std::string msg = Apriltag_detector.getMessage ( i );
        size_t stag = tag.size();
        size_t smsg = msg.size();

        // Checks whether msg ends with tag
        if(    stag <= smsg
            && msg.compare ( smsg - stag, stag, tag) == 0)
        {
            _image_points = Apriltag_detector.getPolygon( i );
            
            if( _state == no_object )
              _state = newly_acquired_object;
            else
              _state = already_acquired_object;

            if (computePose()) return true;

            // Pose could not be computed.
            _state = no_object;
            return false;
        }
    }

    _state = no_object;
    return false;
}

void DetectorAprilTag::drawDebug( vpImage< vpRGBa > &image ) const
{
    if( _state == no_object )
        return; 

    std::array< vpColor, 4 > colors{{ vpColor::red, vpColor::green, vpColor::blue, vpColor::cyan }};

    for( unsigned int i{ 0 } ; i < 4 ; ++i )
        vpDisplay::displayLine( image, _image_points[ i ], _image_points[ i == 3 ? 0 : i + 1 ], colors[ i ], 3 );

    vpDisplay::displayFrame( image, _cMo, _cam_parameters, _tag_size_meters * 2, vpColor::none );

    // FIXME this should be fixed as the display is not correct
    /*
    vpPoint a{ - _tag_size_meters / 2. - 0.022, - _tag_size_meters / 2. - 0.066, 0.0 }; a.project( _cMo );
    vpPoint b{ - _tag_size_meters / 2. - 0.022 + 0.05, - _tag_size_meters / 2. - 0.066, 0.0 }; b.project( _cMo ); 
    vpPoint c{ - _tag_size_meters / 2. - 0.022, - _tag_size_meters / 2. - 0.066 + 0.05, 0.0 }; c.project( _cMo ); 
    vpPoint d{ - _tag_size_meters / 2. - 0.022, - _tag_size_meters / 2. - 0.066, 0.0 + 0.05 }; d.project( _cMo ); 

    vpImagePoint pointA, pointB, pointC, pointD;
    vpMeterPixelConversion::convertPoint( _cam_parameters, a.get_x(), a.get_y(), pointA );
    vpMeterPixelConversion::convertPoint( _cam_parameters, b.get_x(), b.get_y(), pointB );
    vpMeterPixelConversion::convertPoint( _cam_parameters, c.get_x(), c.get_y(), pointC );
    vpMeterPixelConversion::convertPoint( _cam_parameters, d.get_x(), d.get_y(), pointD );

    vpDisplay::displayArrow( image, pointA, pointB, vpColor::red );
    vpDisplay::displayArrow( image, pointA, pointC, vpColor::green );
    vpDisplay::displayArrow( image, pointA, pointD, vpColor::blue );
    */
}

bool DetectorAprilTag::computePose()
{
    if( _state == no_object )
        return false;

    vpPose pose{};
    std::vector< vpPoint > points{ compute3DPoints() };

    // Compute the 2D coord. of the points (in meters) to match the 3D coord. for the pose estimation 
    for( unsigned int i{ 0 } ; i < points.size() ; i++ ) 
    {
        double x{ 0. }, y{ 0. };
        vpPixelMeterConversion::convertPointWithoutDistortion( _cam_parameters, _image_points[ i ], x, y );
        
        // x, y are the coordinates in the image plane, oX, oY, oZ in the world, and X, Y, Z in the camera ref. frame
        points[ i ].set_x( x );
        points[ i ].set_y( y );
        pose.addPoint( points[ i ] );
    }

    if( _state == newly_acquired_object )
    {
        vpPose::computePoseRGBD( _depth, _image_points, _cam_parameters, 
                                 _tag_size_meters, _cMo );
 
        return true;
    }
 
    assert(_state == already_acquired_object);
    pose.computePose( vpPose::VIRTUAL_VS, _cMo );
    _error = pose.computeResidual( _cMo );
    return true;
}

std::vector< vpPoint > DetectorAprilTag::compute3DPoints() const
{
    std::vector< vpPoint > points{};

    vpPoint pt{};
    pt.set_oZ( 0.0 );
    pt.set_oX( - _tag_size_meters / 2. ); pt.set_oY( - _tag_size_meters / 2. ); points.push_back( pt );
    pt.set_oX( _tag_size_meters / 2. ); pt.set_oY( - _tag_size_meters / 2. ); points.push_back( pt );
    pt.set_oX( _tag_size_meters / 2. ); pt.set_oY( _tag_size_meters / 2. ); points.push_back( pt );
    pt.set_oX( - _tag_size_meters / 2. ); pt.set_oY( _tag_size_meters / 2. ); points.push_back( pt );
    
    return points;
}

}
}
