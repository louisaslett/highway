hw_add <- function(x, y) .Call(c_add, as.double(x), as.double(y))
hw_exp <- function(x) .Call(c_exp, as.double(x))
hw_abort <- function(code) .Call(c_abort, as.integer(code))
hw_null_args <- function() .Call(c_null_args)
