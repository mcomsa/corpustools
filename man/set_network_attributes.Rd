% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/semnet_plotting.r
\name{set_network_attributes}
\alias{set_network_attributes}
\title{Set some default network attributes for pretty plotting}
\usage{
set_network_attributes(g, size_attribute = "freq", color_attribute = NA,
  redo_layout = F, edgewidth_coef = 1,
  layout_fun = igraph::layout_with_fr)
}
\arguments{
\item{g}{A graph in the Igraph format.}

\item{size_attribute}{the name of the vertex attribute to be used to set the size of nodes}

\item{color_attribute}{the name of the attribute that is used to select the color}

\item{redo_layout}{if TRUE, force new layout if layout already exists as a graph attribute}

\item{edgewidth_coef}{A coefficient for changing the edge width}

\item{layout_fun}{THe igraph layout function used}
}
\value{
a network in the Igraph format
}
\description{
The purpose of this function is to create some default network attribute options to plot networks in a nice and insightfull way.
}
\examples{
tc = create_tcorpus(c('A B C', 'B C', 'B D'))
g = tc$semnet('token')

igraph::get.edge.attribute(g)
igraph::get.vertex.attribute(g)
\dontrun{plot(g)}
g = set_network_attributes(g, size_attribute = 'freq')
igraph::get.edge.attribute(g)
igraph::get.vertex.attribute(g)
\dontrun{plot(g)}
}
