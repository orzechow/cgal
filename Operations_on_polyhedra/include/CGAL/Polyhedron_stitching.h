// Copyright (c) 2014 GeometryFactory (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
//
// Author(s)     : Sebastien Loriot


#ifndef CGAL_POLYHEDRON_STITCHING_H
#define CGAL_POLYHEDRON_STITCHING_H

#include <CGAL/Modifier_base.h>
#include <CGAL/HalfedgeDS_decorator.h>
#include <vector>
#include <set>

namespace CGAL{

namespace Polyhedron_stitching{

template <class Halfedge_handle, class Point_3>
struct Less_for_halfedge{
  bool operator()( Halfedge_handle h1, Halfedge_handle h2 ) const
  {
    const Point_3& s1=h1->opposite()->vertex()->point();
    const Point_3& t1=h1->vertex()->point();
    const Point_3& s2=h2->opposite()->vertex()->point();
    const Point_3& t2=h2->vertex()->point();
    return
    ( s1 < t1?  std::make_pair(s1,t1) : std::make_pair(t1, s1) )
    <
    ( s2 < t2?  std::make_pair(s2,t2) : std::make_pair(t2, s2) );
  }
};

template <class LessHedge, class Polyhedron, class OutputIterator>
OutputIterator
detect_duplicated_boundary_edges
(Polyhedron& P, OutputIterator out, LessHedge less_hedge)
{
  typedef typename Polyhedron::Halfedge_handle Halfedge_handle;

  P.normalize_border();

  typedef
    std::set<Halfedge_handle, LessHedge > Border_halfedge_set;
  Border_halfedge_set border_halfedge_set(less_hedge);
  for (typename Polyhedron::Halfedge_iterator
          it=P.border_halfedges_begin(), it_end=P.halfedges_end();
          it!=it_end; ++it)
  {
    if ( !it->is_border() ) continue;
    typename Border_halfedge_set::iterator set_it;
    bool insertion_ok;
    CGAL::cpp11::tie(set_it,insertion_ok)=
      border_halfedge_set.insert(it);

    if ( !insertion_ok ) *out++=std::make_pair(*set_it, it);
  }
  return out;
}

template <class Polyhedron>
struct Naive_border_stitching_modifier:
  CGAL::Modifier_base<typename Polyhedron::HalfedgeDS>
{
  typedef typename Polyhedron::HalfedgeDS HDS;
  typedef typename HDS::Halfedge_handle Halfedge_handle;
  typedef typename HDS::Vertex_handle Vertex_handle;
  typedef typename HDS::Halfedge::Base HBase;

  std::vector <std::pair<Halfedge_handle,Halfedge_handle> >& hedge_pairs_to_stitch;

  Naive_border_stitching_modifier(
    std::vector< std::pair<Halfedge_handle,Halfedge_handle> >&
      hedge_pairs_to_stitch_) :hedge_pairs_to_stitch(hedge_pairs_to_stitch_)
  {}

  void update_target_vertex(Halfedge_handle h,
                            Vertex_handle v_kept,
                            CGAL::HalfedgeDS_decorator<HDS>& decorator)
  {
    Halfedge_handle start=h;
    do{
      decorator.set_vertex(h, v_kept);
      h=h->next()->opposite();
    } while( h!=start );
  }


  void operator() (HDS& hds)
  {
    std::size_t nb_hedges=hedge_pairs_to_stitch.size();

    std::set <Halfedge_handle> hedges_to_stitch;
    for (std::size_t k=0; k<nb_hedges; ++k)
    {
      hedges_to_stitch.insert( hedge_pairs_to_stitch[k].first );
      hedges_to_stitch.insert( hedge_pairs_to_stitch[k].second );
    }

    std::vector<Vertex_handle> vertices_to_delete;
    CGAL::HalfedgeDS_decorator<HDS> decorator(hds);
    for (std::size_t k=0; k<nb_hedges; ++k)
    {
      Halfedge_handle h1=hedge_pairs_to_stitch[k].first;
      Halfedge_handle h2=hedge_pairs_to_stitch[k].second;

      CGAL_assertion( h1->is_border() );
      CGAL_assertion( h2->is_border() );
      CGAL_assertion( !h1->opposite()->is_border() );
      CGAL_assertion( !h2->opposite()->is_border() );

    /// Merge the vertices
      Vertex_handle h1_tgt=h1->vertex();
      Vertex_handle h2_src=h2->opposite()->vertex();

      //update vertex pointers: target of h1 vs source of h2
      if ( h1_tgt != h2_src )
      {
        //we remove h2->opposite()->vertex()
        vertices_to_delete.push_back( h2_src );
        update_target_vertex(h2->opposite(), h1_tgt, decorator);
        decorator.set_vertex_halfedge(h1_tgt, h1);
      }
      else
        decorator.set_vertex_halfedge(h1_tgt, h1);

      Vertex_handle h1_src=h1->opposite()->vertex();
      Vertex_handle h2_tgt=h2->vertex();
      //update vertex pointers: target of h1 vs source of h2
      if ( h1_src!= h2_tgt )
      {
        CGAL_assertion( h1_src->point() == h2_tgt->point() );
        //we remove h1->opposite()->vertex()
        vertices_to_delete.push_back( h1_src );
        update_target_vertex(h1->opposite(), h2_tgt, decorator);
        decorator.set_vertex_halfedge(h2_tgt, h1->opposite());
      }
      else
        decorator.set_vertex_halfedge(h1_src, h1->opposite());

    ///update next/prev of neighbor halfedges
      if ( hedges_to_stitch.find(h1->next())==hedges_to_stitch.end() )
      {
        CGAL_assertion( hedges_to_stitch.find(h2->prev())==hedges_to_stitch.end() );
        Halfedge_handle prev=h2->prev();
        Halfedge_handle next=h1->next();
        prev->HBase::set_next(next);
        decorator.set_prev(next, prev);
      }
      else
      {
        CGAL_assertion( hedges_to_stitch.find(h2->prev())!=hedges_to_stitch.end() );
      }

      if ( hedges_to_stitch.find(h2->next())==hedges_to_stitch.end() )
      {
        CGAL_assertion( hedges_to_stitch.find(h1->prev())==hedges_to_stitch.end() );
        Halfedge_handle prev=h1->prev();
        Halfedge_handle next=h2->next();
        prev->HBase::set_next(next);
        decorator.set_prev(next, prev);
      }
      else
      {
        CGAL_assertion( hedges_to_stitch.find(h1->prev())!=hedges_to_stitch.end() );
      }

      //we are going to remove h2 and its opposite
      //set face-halfedge relationship
      decorator.set_face(h1,h2->opposite()->face());
      decorator.set_face_halfedge(h1->face(),h1);
      //update next/prev pointers
      Halfedge_handle tmp=h2->opposite()->prev();
      tmp->HBase::set_next(h1);
      decorator.set_prev(h1,tmp);
      tmp=h2->opposite()->next();
      h1->HBase::set_next(tmp);
      decorator.set_prev(tmp,h1);

     ///remove h2
      hds.edges_erase(h2);
    }

    //remove the extra vertices
    for(typename std::vector<Vertex_handle>::iterator
          itv=vertices_to_delete.begin(),itv_end=vertices_to_delete.end();
          itv!=itv_end; ++itv)
    {
      hds.vertices_erase(*itv);
    }
  }
};

} //end of namespace Polyhedron_stitching

/// Stitches together border halfedges in a polyhedron.
/// The halfedge to be stitched are provided in `hedge_pairs_to_stitch`.
/// Foreach pair `p` in this vector, p.second and its opposite will be removed
/// from `P`.
/// The vertices that get removed from `P` are selected as follow:
/// The pair of halfedges in `hedge_pairs_to_stitch` are processed linearly.
/// Let `p` be such a pair.
/// If the target of p.first has not been marked for deletion,
/// then the source of p.second is.
/// If the target of p.second has not been marked for deletion,
/// then the source of p.first is.
template <class Polyhedron>
void polyhedron_stitching(
  Polyhedron& P,
  std::vector <std::pair<typename Polyhedron::Halfedge_handle,
                         typename Polyhedron::Halfedge_handle> >&
    hedge_pairs_to_stitch)
{
  Polyhedron_stitching::Naive_border_stitching_modifier<Polyhedron>
    modifier(hedge_pairs_to_stitch);
  P.delegate(modifier);
}

/// Same as above but the pair of halfedges to be stitched are found
/// using `less_hedge`. Two halfedges `h1` and `h2` are set to be stitched
/// if `less_hedge(h1,h2)=less_hedge(h2,h1)=true`.
/// `LessHedge` is a key comparison function that is used to sort halfedges
template <class Polyhedron, class LessHedge>
void polyhedron_stitching(Polyhedron& P, LessHedge less_hedge)
{
  typedef typename Polyhedron::Halfedge_handle Halfedge_handle;

  std::vector <std::pair<Halfedge_handle,Halfedge_handle> > hedge_pairs_to_stitch;
  Polyhedron_stitching::detect_duplicated_boundary_edges(
    P, std::back_inserter(hedge_pairs_to_stitch), less_hedge );
  polyhedron_stitching(P, hedge_pairs_to_stitch);
}

/// Same as above using the sorted pair of vertices incidents to the halfedges
/// as comparision
template <class Polyhedron>
void polyhedron_stitching(Polyhedron& P)
{
  typedef typename Polyhedron::Halfedge_handle Halfedge_handle;
  typedef typename Polyhedron::Vertex::Point_3 Point_3;

  Polyhedron_stitching::Less_for_halfedge<Halfedge_handle, Point_3> less_hedge;
  polyhedron_stitching(P, less_hedge);
}

} //end of namespace CGAL


#endif //CGAL_POLYHEDRON_STITCHING_H
