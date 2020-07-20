#pragma once

#include "FastBVH/BVH.h"
#include <vector>

namespace FastBVH {

//! \brief Flags used to configure traverse() method of the BVH.
enum TraverserFlags
{
  //! If any intersection exists, return immediately. The Intersection data is not populated.
  OnlyTestOcclusion = 1,
};

//! \brief Used for traversing a BVH and checking for ray-primitive intersections.
//! \tparam Float The floating point type used by vector components.
//! \tparam Primitive The type of the primitive that the BVH was made with.
//! \tparam Intersector The type of the primitive intersector.
//! \tparam Flags The flags which configure traversal. By default, no flags are active.
template <
    typename Float,
    typename Primitive,
    typename Intersector,
    TraverserFlags Flags = TraverserFlags(0)>
class Traverser final
{
  const BVH<Float, Primitive>& bvh;
  Intersector intersector;

 public:
  //! Constructs a new BVH traverser.
  //! \param bvh_ The BVH to be traversed.
  constexpr Traverser(const BVH<Float, Primitive>& bvh_, const Intersector& intersector_) noexcept
      : bvh(bvh_), intersector(intersector_) {}
  //! Traces single ray through the BVH, getting a list of intersected primitives.
  //! \param ray The ray to be traced.
  //! \return An intersection instance.
  //! It may or may not be valid, based on whether or not the ray made a collision.
  std::vector<const Primitive*> traverse(const OptSegment& segment) const;
};

//! \brief Contains implementation details for the @ref Traverser class.
namespace TraverserImpl {

//! \brief Node for storing state information during traversal.
template <typename Float>
struct Traversal final
{
  //! The index of the node to be traversed.
  uint32_t i;

  //! Minimum hit time for this node.
  Float mint;

  //! Constructs an uninitialized instance of a traversal context.
  constexpr Traversal() noexcept {}

  //! Constructs an initialized traversal context.
  //! \param i_ The index of the node to be traversed.
  constexpr Traversal(int i_, Float mint_) noexcept : i(i_), mint(mint_) {}
};

}  // namespace TraverserImpl

template <
    typename Float,
    typename Primitive,
    typename Intersector,
    TraverserFlags Flags>
std::vector<const Primitive*>
Traverser<Float, Primitive, Intersector, Flags>
    ::traverse(const OptSegment& segment) const
{
  using Traversal = TraverserImpl::Traversal<Float>;

  // List pointers to intersected primitives.
  std::vector<const Primitive*> IntersectedPrimitives;

  // Bounding box min-t/max-t for left/right children at some point in the tree
  Float bbhits[4];
  int32_t closer, other;

  // Working set
  // WARNING : The working set size is relatively small here, should be made dynamic or template-configurable
  Traversal todo[64];
  int32_t stackptr = 0;

  // "Push" on the root node to the working set
  todo[stackptr].i = 0;
  todo[stackptr].mint = -9999999.f;

  const auto nodes = bvh.getNodes();

  auto build_prims = bvh.getPrimitives();

  while (stackptr >= 0)
  {
    // Pop off the next node to work on.
    int ni = todo[stackptr].i;
    Float near = todo[stackptr].mint;
    stackptr--;
    const auto& node(nodes[ni]);

    // Is leaf -> Intersect
    if (node.isLeaf())
    {
      for (uint32_t o = 0; o < node.primitive_count; ++o)
      {
        const auto& obj = build_prims[node.start + o];
        auto current = intersector(*obj, segment);
        if (current)
        {
          IntersectedPrimitives.emplace_back(current.IntersectedP);
          // If we're only testing occlusion, then return true on any hit.
          if (Flags & TraverserFlags::OnlyTestOcclusion)
          {
            return IntersectedPrimitives;
          }
        }
      }
    }
    else 
    {  // Not a leaf

      bool hitc0 = nodes[ni + 1].bbox.intersect(segment, bbhits, bbhits + 1);
      bool hitc1 = nodes[ni + node.right_offset].bbox.intersect(segment, bbhits + 2, bbhits + 3);

      // Did we hit both nodes?
      if (hitc0 && hitc1)
      {
        // We assume that the left child is a closer hit...
        closer = ni + 1;
        other = ni + node.right_offset;

        // ... If the right child was actually closer, swap the relevant values.
        if (bbhits[2] < bbhits[0])
        {
          std::swap(bbhits[0], bbhits[2]);
          std::swap(bbhits[1], bbhits[3]);
          std::swap(closer, other);
        }

        // It's possible that the nearest object is still in the other side, but
        // we'll check the further-away node later...

        // Push the farther first
        todo[++stackptr] = Traversal(other, bbhits[2]);

        // And now the closer (with overlap test)
        todo[++stackptr] = Traversal(closer, bbhits[0]);
      }

      else if (hitc0)
      {
        todo[++stackptr] = Traversal(ni + 1, bbhits[0]);
      }

      else if (hitc1)
      {
        todo[++stackptr] = Traversal(ni + node.right_offset, bbhits[2]);
      }
    }
  }
  return IntersectedPrimitives;
}
}  // namespace FastBVH