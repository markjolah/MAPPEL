import numpy as np
import mappel

M=mappel.Gauss2DMLE([10,7],[0.9,1.2])
N=10
thetas = M.sample_prior(N)
ims = M.simulate_image(thetas)

theta = M.sample_prior()
im = M.simulate_image(theta)




