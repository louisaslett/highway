\name{{{P}}-functions}
\alias{{{P}}-functions}
\alias{hw_add}
\alias{hw_exp}
\alias{hw_abort}
\alias{hw_null_args}
\title{Test Functions of a 'highway' Consumer Package Written in C}
\description{Wrappers around C-callable 'Highway' kernels.}
\usage{
hw_add(x, y)
hw_exp(x)
hw_abort(code)
hw_null_args()
}
\arguments{
  \item{x, y}{Double vectors.}
  \item{code}{Integer abort code (0 means no abort).}
}
\value{See the C sources.}
\examples{
hw_exp(c(0, 1, NA))
}
