\name{consumerRcpp-functions}
\alias{consumerRcpp-functions}
\alias{rcpp_hwy_exp}
\alias{rcpp_hwy_target}
\alias{rcpp_hwy_abort}
\title{Test Functions of a 'highway' Consumer Package Using 'Rcpp'}
\description{'Rcpp' wrappers around dispatched 'Highway' kernels.}
\usage{
rcpp_hwy_exp(x)
rcpp_hwy_target()
rcpp_hwy_abort(code)
}
\arguments{
  \item{x}{A double vector.}
  \item{code}{Integer abort code (0 means no abort).}
}
\value{See the C++ sources.}
\examples{
rcpp_hwy_exp(c(0, 1, NA))
rcpp_hwy_target()
}
