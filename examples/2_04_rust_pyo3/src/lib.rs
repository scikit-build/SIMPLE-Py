use pyo3::prelude::*;

/// A Python module implemented in Rust.
#[pymodule]
mod pyo3_example {
    use pyo3::prelude::*;

    /// Formats the sum of two numbers as string.
    #[pyfunction]
    fn sum_as_string(a: usize, b: usize) -> PyResult<String> {
        Ok((a + b).to_string())
    }

    /// Counts primes below `limit` by trial division.
    #[pyfunction]
    fn count_primes(py: Python<'_>, limit: u64) -> u64 {
        // Release the GIL so other Python threads can run during the hot loop.
        py.detach(|| {
            let mut count = 0;
            for n in 2..limit {
                let mut is_prime = true;
                let mut d = 2;
                while d * d <= n {
                    if n % d == 0 {
                        is_prime = false;
                        break;
                    }
                    d += 1;
                }
                if is_prime {
                    count += 1;
                }
            }
            count
        })
    }

    /// A 2D point, exposed to Python as a class.
    #[pyclass]
    struct Point {
        #[pyo3(get)]
        x: f64,
        #[pyo3(get)]
        y: f64,
    }

    #[pymethods]
    impl Point {
        #[new]
        fn new(x: f64, y: f64) -> Self {
            Point { x, y }
        }

        /// Distance from the origin.
        fn magnitude(&self) -> f64 {
            (self.x * self.x + self.y * self.y).sqrt()
        }

        fn __repr__(&self) -> String {
            // {:?} keeps the decimal point on whole floats: 1.0, not 1.
            format!("Point(x={:?}, y={:?})", self.x, self.y)
        }
    }

    /// Divides `a` by `b`, raising ZeroDivisionError like Python's `/`.
    #[pyfunction]
    fn checked_div(a: f64, b: f64) -> PyResult<f64> {
        use pyo3::exceptions::PyZeroDivisionError;

        if b == 0.0 {
            return Err(PyZeroDivisionError::new_err("division by zero"));
        }
        Ok(a / b)
    }
}
