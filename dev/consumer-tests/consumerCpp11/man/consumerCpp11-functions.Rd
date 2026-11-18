\name{consumerCpp11-functions}
\alias{consumerCpp11-functions}
\alias{cpp11_hwy_exp}
\alias{cpp11_hwy_target}
\alias{cpp11_hwy_abort}
\title{Test Functions of a 'highway' Consumer Package Using 'cpp11'}
\description{'cpp11' wrappers around statically dispatched 'Highway' code.}
\usage{
cpp11_hwy_exp(x)
cpp11_hwy_target()
cpp11_hwy_abort(code)
}
\arguments{
  \item{x}{A double vector.}
  \item{code}{Integer abort code (0 means no abort).}
}
\value{See the C++ sources.}
\examples{
cpp11_hwy_exp(c(0, 1, NA))
cpp11_hwy_target()
}
