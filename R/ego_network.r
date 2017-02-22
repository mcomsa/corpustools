#' Create an ego network
#'
#' Create an ego network from an igraph object.
#'
#' The function is similar to the ego function in igraph, but with some notable differences. Firstly, if multiple vertex_names are given, the ego network for both is given in 1 network (whereas igraph creates a list of networks). Secondly, the min_weight and top_edges parameters can be used to focus on the strongest edges.
#'
#' @param g an igraph object
#' @param vertex_names a character string with the names of the ego vertices/nodes
#' @param depth the number of degrees from the ego vertices/nodes that are included. 1 means that only the direct neighbours are included
#' @param only_filter_vertices if True, the algorithm will only filter out vertices/nodes that are not in the ego network. If False (default) then it also filters out the edges.
#' @param weight_attr the name of the edge attribute. if NA, no weight is used, and min_weight and top_edges are ignored
#' @param min_weight a number indicating the minimum weight
#' @param top_edges for each vertex within the given depth, only keep the top n edges with the strongest edge weight. Can also be a vector of the same length as the depth value, in which case a different value is used at each level: first value for level 1, second value for level 2, etc.
#' @param max_edges_level the maximum number of edges to be added at each level of depth.
#' @param directed if the network is directed, specify whether 'out' degrees or 'in' degrees are used
#'
#' @return
#' @export
ego_semnet <- function(g, vertex_names, depth=1, only_filter_vertices=T, weight_attr='weight', min_weight=NULL, top_edges=NULL, max_edges_level=NULL, directed=c('out','in')){
  directed = match.arg(directed)

  missing = vertex_names[!vertex_names %in% V(g)$name]
  if(length(missing) == length(vertex_names)) stop('None of the given vertex_names exist in g')
  if(length(missing) > 0) warning(sprintf('Some of the given vertex_names do not exist in g: [%s]', paste(missing, collapse=', ')))

  delete.edges(g, E(g))
  if(!is.na(weight_attr)) {
    adj = get.adjacency(g, type='both', attr = weight_attr)
  } else {
    adj = get.adjacency(g, type='both')
    min_weight = NA; top_edges = NA
  }
  adj = as(adj, 'dgTMatrix')
  if(is.directed(g)){
    if(directed == 'out') dt = summary(adj)
    if(directed == 'in') dt = summary(t(adj))
  } else {
    dt = summary(adj)
  }
  vnames = as.factor(colnames(adj))
  dt = data.table(x=dt$i, y=dt$j, weight=dt$x, key='x') ## as data.table

  vertex_ids = which(V(g)$name %in% vertex_names)
  ego = build_ego_network(dt, vertex_ids, level=1, depth=depth, min_weight=min_weight, top_edges=top_edges, max_edges_level=max_edges_level)
  if(only_filter_vertices){
    i = unique(c(ego$x, ego$y))
    g = igraph::delete.vertices(g, which(!1:vcount(g) %in% i))
  } else {
    i = get.edge.ids(g, vp=rbind(ego$x, ego$y))
    g = igraph::delete_edges(g, which(!1:ecount(g) %in% i))
    g = igraph::delete_vertices(g, which(degree(g) == 0))
  }
  g
}

build_ego_network <- function(dt, vertex_ids, level, depth, min_weight, top_edges, max_edges_level){
  ego = dt[J(vertex_ids),]
  if(!is.null(min_weight)) ego = ego[ego$weight >= min_weight,]
  if(!is.null(top_edges)){
    thres = if(length(top_edges) == depth) top_edges[depth] else top_edges
    ego = ego[order(ego$x, -ego$weight)]
    top = local_position(1:nrow(ego), ego$x)
    ego = ego[top <= thres,]
  }
  if(!is.null(max_edges_level)) ego = head(ego[order(-ego$weight)], max_edges_level)

  new_vertex_ids = unique(ego$y)
  new_vertex_ids = new_vertex_ids[!new_vertex_ids %in% vertex_ids]
  if(level < depth) ego = rbind(ego, build_ego_network(dt, vertex_ids=new_vertex_ids,
                                                           level=level+1, depth=depth,
                                                           min_weight=min_weight, top_edges=top_edges,
                                                           max_edges_level=max_edges_level)) ## build ego network recursively
  ego
}
