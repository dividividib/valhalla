#include <iostream>

#include <valhalla/proto/trippath.pb.h>
#include <valhalla/midgard/util.h>

#include "odin/enhancedtrippath.h"

using namespace valhalla::midgard;

namespace valhalla {
namespace odin {

///////////////////////////////////////////////////////////////////////////////
// EnhancedTripPath

EnhancedTripPath::EnhancedTripPath() {
}

EnhancedTripPath_Node* EnhancedTripPath::GetEnhancedNode(const int node_index) {
  EnhancedTripPath_Node* node =
      static_cast<EnhancedTripPath_Node*>(mutable_node(node_index));
  node->set_last_node(IsLastNodeIndex(node_index));
  return node;
}

EnhancedTripPath_Edge* EnhancedTripPath::GetPrevEdge(const int node_index,
                                                     int delta) {
  int index = node_index - delta;
  if (IsValidNodeIndex(index))
    return static_cast<EnhancedTripPath_Edge*>(mutable_node(index)->mutable_edge(
        0));
  else
    return nullptr;
}

EnhancedTripPath_Edge* EnhancedTripPath::GetCurrEdge(const int node_index) {
  return GetNextEdge(node_index, 0);
}

EnhancedTripPath_Edge* EnhancedTripPath::GetNextEdge(const int node_index,
                                                     int delta) {
  int index = node_index + delta;
  if (IsValidNodeIndex(index) && !IsLastNodeIndex(index))
    return static_cast<EnhancedTripPath_Edge*>(mutable_node(index)->mutable_edge(
        0));
  else
    return nullptr;
}

bool EnhancedTripPath::IsValidNodeIndex(int node_index) const {
  if ((node_index >= 0) && (node_index < node_size()))
    return true;
  return false;
}

bool EnhancedTripPath::IsFirstNodeIndex(int node_index) const {
  if (node_index == 0)
    return true;
  return false;
}

bool EnhancedTripPath::IsLastNodeIndex(int node_index) const {
  if (IsValidNodeIndex(node_index) && (node_index == (node_size() - 1)))
    return true;
  return false;
}

int EnhancedTripPath::GetLastNodeIndex() const {
  return (node_size() - 1);
}

///////////////////////////////////////////////////////////////////////////////
// EnhancedTripPath_Edge

EnhancedTripPath_Edge::EnhancedTripPath_Edge() {
}

bool EnhancedTripPath_Edge::IsUnnamed() const {
  if (name_size() == 0)
    return true;
  return false;
}

bool EnhancedTripPath_Edge::IsHighway() const {
  if ((road_class() == TripPath_RoadClass_kMotorway) && (!ramp()))
    return true;
  return false;
}

std::string EnhancedTripPath_Edge::ToString() const {
  std::string str;
  str.reserve(128);

  str += "name=";
  if (name_size() == 0) {
    str += "unnamed";
  } else {
    bool is_first = true;
    for (const auto& name : this->name()) {
      if (is_first)
        is_first = false;
      else
        str += "/";
      str += name;
    }
  }

  str += " | length=";
  str += std::to_string(length());

  str += " | speed=";
  str += std::to_string(speed());

  str += " | road_class=";
  str += std::to_string(road_class());

  str += " | begin_heading=";
  str += std::to_string(begin_heading());

  str += " | end_heading=";
  str += std::to_string(end_heading());

  str += " | begin_shape_index=";
  str += std::to_string(begin_shape_index());

  str += " | end_shape_index=";
  str += std::to_string(end_shape_index());

  str += " | driveability=";
  str += std::to_string(driveability());

  str += " | ramp=";
  str += std::to_string(ramp());

  str += " | turn_channel=";
  str += std::to_string(turn_channel());

  str += " | ferry=";
  str += std::to_string(ferry());

  str += " | rail_ferry=";
  str += std::to_string(rail_ferry());

  str += " | toll=";
  str += std::to_string(toll());

  str += " | unpaved=";
  str += std::to_string(unpaved());

  str += " | tunnel=";
  str += std::to_string(tunnel());

  str += " | bridge=";
  str += std::to_string(bridge());

  str += " | roundabout=";
  str += std::to_string(roundabout());

  str += " | internal_link=";
  str += std::to_string(internal_link());

  str += " | end_node_index=";
  str += std::to_string(end_node_index());

  return str;
}

///////////////////////////////////////////////////////////////////////////////
// EnhancedTripPath_Node

bool EnhancedTripPath_Node::last_node() const {
  return last_node_;
}

void EnhancedTripPath_Node::set_last_node(bool last_node) {
  last_node_ = last_node;
}

bool EnhancedTripPath_Node::HasIntersectingEdges() const {
  // Last index is special since the edges do not include a path edge
  if (last_node_)
    return (edge().size() > 0);
  return (edge().size() > 1);
}

size_t EnhancedTripPath_Node::GetIntersectingEdgesCount() const {
  if (last_node_)
    return (edge().size());
  return (edge().size() - 1);
}

EnhancedTripPath_Edge* EnhancedTripPath_Node::GetIntersectingEdge(
    size_t index) {
  return static_cast<EnhancedTripPath_Edge*>(mutable_edge(
      last_node_ ? index : ++index));
}

void EnhancedTripPath_Node::CalculateRightLeftIntersectingEdgeCounts(
    uint32_t from_heading, uint32_t& right_count, uint32_t& left_count) {
  right_count = 0;
  left_count = 0;

  // No turn at last node - just return
  if (last_node_)
    return;

  uint32_t path_turn_degree = GetTurnDegree(from_heading,
                                            edge(0).begin_heading());
  for (size_t i = 1; i <= GetIntersectingEdgesCount(); ++i) {
    uint32_t intersecting_turn_degree = GetTurnDegree(from_heading,
                                                      edge(i).begin_heading());
    if (path_turn_degree > 180) {
      if ((intersecting_turn_degree > path_turn_degree)
          || (intersecting_turn_degree < 180)) {
        ++right_count;
      }
      else if ((intersecting_turn_degree < path_turn_degree)
          && (intersecting_turn_degree > 180)) {
        ++left_count;
      }
    } else {
      if ((intersecting_turn_degree > path_turn_degree)
          && (intersecting_turn_degree < 180)) {
        ++right_count;
      }
      else if ((intersecting_turn_degree < path_turn_degree)
          || (intersecting_turn_degree > 180)) {
        ++left_count;
      }
    }
  }
}

}
}

