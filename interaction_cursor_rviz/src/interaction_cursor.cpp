/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "interaction_cursor_rviz/interaction_cursor.h"


#include "rviz/display_context.h"
#include "rviz/frame_manager.h"
#include "rviz/ogre_helpers/shape.h"
#include "rviz/ogre_helpers/axes.h"
#include "rviz/ogre_helpers/custom_parameter_indices.h"
#include "rviz/properties/float_property.h"
#include "rviz/properties/tf_frame_property.h"
#include "rviz/properties/ros_topic_property.h"

#include "rviz/selection/forwards.h"
#include "rviz/selection/selection_handler.h"
#include "rviz/selection/selection_manager.h"

#include "rviz/default_plugin/interactive_markers/interactive_marker_control.h"
#include "rviz/default_plugin/interactive_markers/interactive_marker.h"

#include <OGRE/OgreSceneNode.h>
#include <OGRE/OgreSceneManager.h>
#include <OGRE/OgreRenderable.h>

#include <boost/bind.hpp>


#if( OGRE_VERSION_MINOR < 8 )
// TODO This is a TERRIBLE work around to add this function to Ogre versions before 1.8.
namespace Ogre {
class MyRenderable : public Renderable
{
public:
  MyRenderable() : Renderable() {}
  ~MyRenderable() {}
  bool hasCustomParameter(size_t index) const
  {
    return mCustomParameters.find(index) != mCustomParameters.end();
  }
};
}
#endif

namespace rviz
{

// Some convenience functions for Ogre / geometry_msgs conversions
static inline Ogre::Vector3 vectorFromMsg(const geometry_msgs::Point &m) { return Ogre::Vector3(m.x, m.y, m.z); }
static inline Ogre::Vector3 vectorFromMsg(const geometry_msgs::Vector3 &m) { return Ogre::Vector3(m.x, m.y, m.z); }
static inline geometry_msgs::Point pointOgreToMsg(const Ogre::Vector3 &o)
{
  geometry_msgs::Point m;
  m.x = o.x; m.y = o.y; m.z = o.z;
  return m;
}
static inline void pointOgreToMsg(const Ogre::Vector3 &o, geometry_msgs::Point &m)  { m.x = o.x; m.y = o.y; m.z = o.z; }

static inline geometry_msgs::Vector3 vectorOgreToMsg(const Ogre::Vector3 &o)
{
  geometry_msgs::Vector3 m;
  m.x = o.x; m.y = o.y; m.z = o.z;
  return m;
}
static inline void vectorOgreToMsg(const Ogre::Vector3 &o, geometry_msgs::Vector3 &m) { m.x = o.x; m.y = o.y; m.z = o.z; }

//// -----------------------------------------------------------------------------
///** Visitor object that can be used to iterate over a collection of Renderable
//instances abstractly.
//@remarks
//Different scene objects use Renderable differently; some will have a
//single Renderable, others will have many. This visitor interface allows
//classes using Renderable to expose a clean way for external code to
//get access to the contained Renderable instance(s) that it will
//eventually add to the render queue.
//@par
//To actually have this method called, you have to call a method on the
//class containing the Renderable instances. One example is
//MovableObject::visitRenderables.
//*/
//class MyVisitor : public Ogre::Renderable::Visitor
//{
//public:
//  MyVisitor()
//    : disp_(0) {}

//  /** Virtual destructor needed as class has virtual methods. */
//  virtual ~MyVisitor() { }
//  /** Generic visitor method.
//  @param rend The Renderable instance being visited
//  @param lodIndex The LOD index to which this Renderable belongs. Some
//    objects support LOD and this will tell you whether the Renderable
//    you're looking at is from the top LOD (0) or otherwise
//  @param isDebug Whether this is a debug renderable or not.
//  @param pAny Optional pointer to some additional data that the class
//    calling the visitor may populate if it chooses to.
//  */
//  virtual void visit(Ogre::Renderable* rend, ushort lodIndex, bool isDebug, Ogre::Any* pAny = 0)
//  {

//#if( OGRE_VERSION_MINOR < 8 )
//    // This static cast is INCREDIBLY unsafe, but is a temporary work around since Ogre < 1.8 does not have this.
//    Ogre::MyRenderable* myrend = static_cast<Ogre::MyRenderable*>(rend);
//    if( !myrend->hasCustomParameter(PICK_COLOR_PARAMETER) ) return;
//#else
//    if( !rend->hasCustomParameter(PICK_COLOR_PARAMETER) ) return;
//#endif
//    Ogre::Vector4 vec = rend->getCustomParameter(PICK_COLOR_PARAMETER);

//    rviz::SelectionManager* sm = disp_->getDisplayContext()->getSelectionManager();
//    if(sm)
//    {
//      Ogre::ColourValue colour(vec.x, vec.y, vec.z, 1.0);
//      CollObjectHandle handle = colorToHandle(colour);

//      rviz::SelectionHandler* handler = sm->getHandler(handle);
//      if(handle)
//      {
//        InteractiveObjectWPtr ptr = handler->getInteractiveObject();

//        // Don't do anything to a control if we are already grabbing its parent marker.
//        if(ptr.lock() == disp_->grabbed_object_.lock())
//          return;

//        //weak_ptr<Fruit> fruit = weak_ptr<Fruit>(dynamic_pointer_cast<Fruit>(food.lock());
//        boost::shared_ptr<InteractiveMarkerControl> control = boost::dynamic_pointer_cast<InteractiveMarkerControl>(ptr.lock());
//        if(control && control->getVisible())
//        {
//          control->setHighlight(InteractiveMarkerControl::HOVER_HIGHLIGHT);
//          disp_->highlighted_objects_.insert(ptr);
//        }
//      }
//    }
//  }

//  rviz::InteractionCursorDisplay* disp_;
//};


/** This optional class allows you to receive per-result callbacks from
    SceneQuery executions instead of a single set of consolidated results.
@remarks
    You should override this with your own subclass. Note that certain query
    classes may refine this listener interface.
*/
class MySceneQueryListener : public Ogre::SceneQueryListener
{
public:
  MySceneQueryListener() : disp_(0) { }
  virtual ~MySceneQueryListener() { }
    /** Called when a MovableObject is returned by a query.
    @remarks
        The implementor should return 'true' to continue returning objects,
        or 'false' to abandon any further results from this query.
    */
  virtual bool queryResult(Ogre::MovableObject* object)
  {
    // Do a simple attempt to refine the search by checking the "local" oriented bounding box:
    const Ogre::AxisAlignedBox& local_bb = object->getBoundingBox();
    Ogre::Sphere local_query_sphere = query_sphere_;
    local_query_sphere.setCenter(object->getParentNode()->convertWorldToLocalPosition(local_query_sphere.getCenter()));
    local_query_sphere.setRadius(local_query_sphere.getRadius()*2); // magic number... not sure why this is needed
    // If the sphere doesn't at least intersect the local bounding box, we skip this result.
    if(!local_bb.intersects(local_query_sphere)) return true;


    // If we got to here, we are assuming that we have satisfactory intersection.
    //ROS_INFO("Sphere collides with MoveableObject [%s].",  object->getName().c_str());

    // The new rviz change adds a user object binding called "pick_handle" to objects when the pick color is set.
    Ogre::Any handle_any = object->getUserObjectBindings().getUserAny( "pick_handle" );

    if( handle_any.isEmpty() )
    {
      return true;
    }
    // Get the CollObjectHandle
    CollObjectHandle handle = Ogre::any_cast<CollObjectHandle>(handle_any);

    rviz::SelectionHandler* handler = disp_->getDisplayContext()->getSelectionManager()->getHandler(handle);
    if(handle)
    {
      InteractiveObjectWPtr ptr = handler->getInteractiveObject();

      // Don't do anything to a control if we are already grabbing its parent marker.
      if(ptr.lock() == disp_->grabbed_object_.lock())
        return true;

      //weak_ptr<Fruit> fruit = weak_ptr<Fruit>(dynamic_pointer_cast<Fruit>(food.lock());
      boost::shared_ptr<InteractiveMarkerControl> control = boost::dynamic_pointer_cast<InteractiveMarkerControl>(ptr.lock());
      if(control && control->getVisible())
      {
        control->setHighlight(InteractiveMarkerControl::HOVER_HIGHLIGHT);
        disp_->highlighted_objects_.insert(ptr);

        // returning false means no subsequent object will be checked.
        return false;
      }
    }
    return true;
  }


    /** Called when a WorldFragment is returned by a query.
    @remarks
        The implementor should return 'true' to continue returning objects,
        or 'false' to abandon any further results from this query.
    */
    virtual bool queryResult(Ogre::SceneQuery::WorldFragment* fragment)
    {
      //ROS_INFO("Sphere collides with WorldFragment type [%d].", fragment->fragmentType );
      return true;
    }
    rviz::InteractionCursorDisplay* disp_;
    Ogre::Sphere query_sphere_;
};

// -----------------------------------------------------------------------------


InteractionCursorDisplay::InteractionCursorDisplay()
  : Display()
  , nh_("")
  , cursor_shape_(0)
  , dragging_(false)
{
  grabbed_object_.reset();

  update_topic_property_ = new RosTopicProperty( "Update Topic", "",
                                                        ros::message_traits::datatype<interaction_cursor_msgs::InteractionCursorUpdate>(),
                                                        "interaction_cursor_msgs::InteractionCursorUpdate topic to subscribe to.",
                                                        this, SLOT( updateTopic() ));

  show_cursor_shape_property_ = new BoolProperty("Show Cursor", true,
                                                 "Enables display of cursor shape.",
                                                 this, SLOT( updateShape() ));

  shape_scale_property_ = new FloatProperty( "Cursor Radius", 0.05,
                                        "Size of search sphere, in meters.",
                                        this, SLOT( updateShape() ));
  shape_scale_property_->setMin( 0.0001 );

  show_cursor_axes_property_ = new BoolProperty("Show Axes", true,
                                                "Enables display of cursor axes.",
                                                this, SLOT( updateAxes()));

  axes_length_property_ = new FloatProperty( "Axes Length", 0.1,
                                        "Length of each axis, in meters.",
                                        this, SLOT( updateAxes() ));
  axes_length_property_->setMin( 0.0001 );

  axes_radius_property_ = new FloatProperty( "Axes Radius", 0.01,
                                        "Radius of each axis, in meters.",
                                        this, SLOT( updateAxes() ));
  axes_radius_property_->setMin( 0.0001 );
}

InteractionCursorDisplay::~InteractionCursorDisplay()
{
  delete cursor_shape_;
  delete cursor_axes_;
  context_->getSceneManager()->destroySceneNode( cursor_node_ );
}

void InteractionCursorDisplay::updateTopic()
{
  subscriber_update_ = nh_.subscribe<interaction_cursor_msgs::InteractionCursorUpdate>
                              (update_topic_property_->getStdString(), 10,
                              boost::bind(&InteractionCursorDisplay::updateCallback, this, _1));
}

void InteractionCursorDisplay::onInitialize()
{
  //frame_property_->setFrameManager( context_->getFrameManager() );

  cursor_node_ = context_->getSceneManager()->getRootSceneNode()->createChildSceneNode();
  cursor_node_->setVisible( isEnabled() );

  cursor_axes_ = new Axes( scene_manager_, cursor_node_, axes_length_property_->getFloat(), axes_radius_property_->getFloat() );
  cursor_axes_->getSceneNode()->setVisible( show_cursor_axes_property_->getBool() );

  cursor_shape_ = new Shape( Shape::Sphere, context_->getSceneManager(), cursor_node_);
  cursor_shape_->setScale(Ogre::Vector3(0.2f));
  cursor_shape_->setColor(0.3f, 1.0f, 0.3f, 0.5f);
  cursor_shape_->getRootNode()->setVisible( show_cursor_shape_property_->getBool() );

  // Should this happen onEnable and then get killed later?
  updateTopic();
}

void InteractionCursorDisplay::onEnable()
{
  cursor_node_->setVisible( true );
}

void InteractionCursorDisplay::onDisable()
{
  cursor_node_->setVisible( false );
}

void InteractionCursorDisplay::updateAxes()
{
  cursor_axes_->set( axes_length_property_->getFloat(), axes_radius_property_->getFloat() );
  cursor_axes_->getSceneNode()->setVisible( show_cursor_axes_property_->getBool(), true);
  context_->queueRender();
}


void InteractionCursorDisplay::updateShape()
{
  // multiply by 2 because shape_scale sets sphere diameter
  Ogre::Vector3 shape_scale( 2*1.02*shape_scale_property_->getFloat());
  cursor_shape_->setScale( shape_scale );
  cursor_shape_->getRootNode()->setVisible( show_cursor_shape_property_->getBool(), true );
  context_->queueRender();
}

ViewportMouseEvent InteractionCursorDisplay::createMouseEvent(uint8_t button_state)
{
  ViewportMouseEvent event;
  event.viewport = context_->getSceneManager()->getCurrentViewport();

  if(button_state == interaction_cursor_msgs::InteractionCursorUpdate::NONE)
  {
    event.type = QEvent::None;
  }
  if(button_state == interaction_cursor_msgs::InteractionCursorUpdate::GRAB)
  {
    event.type = QEvent::MouseButtonPress;
    event.acting_button = Qt::LeftButton;
  }
  if(button_state == interaction_cursor_msgs::InteractionCursorUpdate::KEEP_ALIVE)
  {
    event.type = QEvent::MouseMove;
    event.buttons_down = event.buttons_down | Qt::LeftButton;
    //event.acting_button = Qt::LeftButton;
  }
  if(button_state == interaction_cursor_msgs::InteractionCursorUpdate::RELEASE)
  {
    event.type = QEvent::MouseButtonRelease;
    event.acting_button = Qt::LeftButton;
  }
  return event;
}

void InteractionCursorDisplay::updateCallback(const interaction_cursor_msgs::InteractionCursorUpdateConstPtr &icu_cptr)
{
  //ROS_INFO("Got a InteractionCursor update!");
  std::string frame = icu_cptr->pose.header.frame_id;
  Ogre::Vector3 position;
  Ogre::Quaternion quaternion;

  if( context_->getFrameManager()->transform(frame, ros::Time(0), icu_cptr->pose.pose, position, quaternion) )
  {
    cursor_node_->setPosition( position );
    cursor_node_->setOrientation( quaternion );
    updateShape();

    Ogre::Sphere sphere(position, shape_scale_property_->getFloat());
    clearOldSelections();

    if(icu_cptr->button_state == icu_cptr->NONE)
    {
      getIntersections(sphere);
    }
    else if(icu_cptr->button_state == icu_cptr->GRAB)
    {
      getIntersections(sphere);
      grabObject(position, quaternion, createMouseEvent(icu_cptr->button_state));
    }
    else if(icu_cptr->button_state == icu_cptr->KEEP_ALIVE)// && dragging_)
    {
      //ROS_INFO("Updating object pose!");
      updateGrabbedObject(position, quaternion, createMouseEvent(icu_cptr->button_state));
    }
    else if(icu_cptr->button_state == icu_cptr->RELEASE)// && dragging_)
    {
      //ROS_INFO("Releasing object!");
      releaseObject(position, quaternion, createMouseEvent(icu_cptr->button_state));
    }
    context_->queueRender();

    setStatus( StatusProperty::Ok, "Transform", "Transform OK" );
  }
  else
  {
    std::string error;
    if( context_->getFrameManager()->transformHasProblems( frame, ros::Time(), error ))
    {
      setStatus( StatusProperty::Error, "Transform", QString::fromStdString( error ));
    }
    else
    {
      setStatus( StatusProperty::Error,
                 "Transform",
                 "Could not transform from [" + QString::fromStdString(frame) + "] to Fixed Frame [" + fixed_frame_ + "] for an unknown reason" );
    }
  }
}

void InteractionCursorDisplay::clearOldSelections()
{
  std::set<InteractiveObjectWPtr>::iterator it;
  for ( it=highlighted_objects_.begin() ; it != highlighted_objects_.end(); it++ )
  {
    InteractiveObjectWPtr ptr = (*it);
    if(!ptr.expired())
    {
      boost::shared_ptr<InteractiveMarkerControl> control = boost::dynamic_pointer_cast<InteractiveMarkerControl>(ptr.lock());
      if(control)
      {
        control->setHighlight(InteractiveMarkerControl::NO_HIGHLIGHT);
        //disp_->highlighted_objects.insert(ptr);
      }
    }
  }
  highlighted_objects_.clear();
}

void InteractionCursorDisplay::getIntersections(const Ogre::Sphere &sphere)
{
  Ogre::SphereSceneQuery* ssq = context_->getSceneManager()->createSphereQuery(sphere);
  MySceneQueryListener listener;
  listener.disp_ = this;
  listener.query_sphere_ = sphere;
  ssq->execute(&listener);
  context_->getSceneManager()->destroyQuery(ssq);
}

void InteractionCursorDisplay::grabObject(const Ogre::Vector3 &position, const Ogre::Quaternion &orientation, const rviz::ViewportMouseEvent &event)
{
  if(highlighted_objects_.begin() == highlighted_objects_.end())
    return;

  InteractiveObjectWPtr ptr = *(highlighted_objects_.begin());
  // Remove the object from the set so that we don't un-highlight it later on accident.
  highlighted_objects_.erase(highlighted_objects_.begin());

  if(!ptr.expired())
  {
    boost::shared_ptr<InteractiveMarkerControl> control = boost::dynamic_pointer_cast<InteractiveMarkerControl>(ptr.lock());
    if(control)
    {
        control->handle3DCursorEvent(event, position);
//      control->setHighlight(InteractiveMarkerControl::ACTIVE_HIGHLIGHT);
//      InteractiveMarker* im = control->getParent();
//      im->startDragging();
//      Ogre::Vector3 r_marker_to_cursor_in_cursor_frame = orientation.Inverse()*(position - im->getPosition());
//      position_offset_at_grab_ = r_marker_to_cursor_in_cursor_frame;
//      orientation_offset_at_grab_ = orientation.Inverse()*im->getOrientation();
//      marker_frame_at_grab_= im->getReferenceFrame();
//      std::string marker_name = im->getName();
//      ROS_INFO("Grabbed marker [%s] with reference frame [%s], position offset [%.3f %.3f %.3f] and quaternion offset [%.2f %.2f %.2f %.2f]",
//               marker_name.c_str(), marker_frame_at_grab_.c_str(),
//               position_offset_at_grab_.x, position_offset_at_grab_.y, position_offset_at_grab_.z,
//               orientation_offset_at_grab_.x, orientation_offset_at_grab_.y, orientation_offset_at_grab_.z, orientation_offset_at_grab_.w);

      grabbed_object_ = ptr;
      //dragging_ = true;

      //disp_->highlighted_objects.insert(ptr);
    }
  }
}

void InteractionCursorDisplay::updateGrabbedObject(const Ogre::Vector3 &position, const Ogre::Quaternion &orientation, const rviz::ViewportMouseEvent &event)
{
  if(!grabbed_object_.expired())
  {
    //ROS_INFO("Locking control...");
    boost::shared_ptr<InteractiveMarkerControl> control = boost::dynamic_pointer_cast<InteractiveMarkerControl>(grabbed_object_.lock());
    if(control)
    {
      //ROS_INFO("updating...");
      control->handle3DCursorEvent(event, position);
//      const std::string& control_name = control->getName();
//      InteractiveMarker* im = control->getParent();
//      Ogre::Vector3 r_marker_to_cursor_in_cursor_frame = position_offset_at_grab_;
//      Ogre::Quaternion marker_orientation = orientation*orientation_offset_at_grab_;
//      Ogre::Vector3 marker_position = orientation*(orientation.Inverse()*position - r_marker_to_cursor_in_cursor_frame);
//      im->setPose( marker_position, marker_orientation, control_name );

    }
  }
  else
  {
    ROS_WARN("Grabbed object weak pointer has expired...");
  }
}

void InteractionCursorDisplay::releaseObject(const Ogre::Vector3 &position, const Ogre::Quaternion &orientation, const rviz::ViewportMouseEvent &event)
{
  if(!grabbed_object_.expired())
  {
    boost::shared_ptr<InteractiveMarkerControl> control = boost::dynamic_pointer_cast<InteractiveMarkerControl>(grabbed_object_.lock());
    if(control)
    {
      ROS_INFO("Releasing object [%s]", control->getName().c_str());
      control->handle3DCursorEvent(event, position);

//      control->setHighlight(InteractiveMarkerControl::NO_HIGHLIGHT);
//      control->getParent()->stopDragging();
//      // Add it back to the set for later un-highlighting.
      highlighted_objects_.insert(grabbed_object_);
    }
  }
  else
  {
    ROS_WARN("Grabbed object seems to have expired before we released it!");
  }
  grabbed_object_.reset();
  //dragging_ = false;


}

//void InteractionCursorDisplay::

void InteractionCursorDisplay::update( float dt, float ros_dt )
{

}

} // namespace rviz

#include <pluginlib/class_list_macros.h>
// namespace, name, type, base type
PLUGINLIB_DECLARE_CLASS( rviz, InteractionCursorDisplay, rviz::InteractionCursorDisplay, rviz::Display )


