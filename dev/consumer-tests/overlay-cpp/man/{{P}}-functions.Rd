\name{{{P}}-functions}
\alias{{{P}}-functions}
\alias{hw_add}
\alias{hw_exp}
\alias{hw_log}
\alias{hw_targets}
\alias{hw_force_target}
\alias{hw_chosen_initialized}
\alias{hw_deinit}
\alias{hw_warm_up}
\alias{hw_set_abort_handler}
\alias{hw_handler_calls}
\alias{hw_abort}
\alias{hw_abort_workers}
\alias{hw_exp_threads}
\title{Test Functions of a 'highway' Consumer Package}
\description{Thin wrappers around the package's 'Highway' kernels and test hooks.}
\usage{
hw_add(x, y)
hw_exp(x)
hw_log(x)
hw_targets()
hw_force_target(target)
hw_chosen_initialized()
hw_deinit()
hw_warm_up()
hw_set_abort_handler(on)
hw_handler_calls()
hw_abort(code)
hw_abort_workers(code, nthreads)
hw_exp_threads(x, nthreads, cold = FALSE)
}
\arguments{
  \item{x, y}{Double vectors.}
  \item{target}{Target name, or \code{""} to reset.}
  \item{on}{Logical.}
  \item{code}{Integer abort code (0 means no abort).}
  \item{nthreads}{Number of threads.}
  \item{cold}{Reset dispatch before the threads start?}
}
\value{See the C++ sources.}
\examples{
hw_exp(c(0, 1, NA))
hw_targets()
}
