/** @file Gauss2DMLE.h
 * @author Mark J. Olah (mjo\@cs.unm DOT edu)
 * @date 2014-2019
 * @brief The class declaration and inline and templated functions for Gauss2DMLE.
 */

#ifndef MAPPEL_GAUSS2DMLE_H
#define MAPPEL_GAUSS2DMLE_H

#include "Mappel/Gauss2DModel.h"
#include "Mappel/PoissonNoise2DObjective.h"
#include "Mappel/MLEstimator.h"
#include "Mappel/model_methods.h"

namespace mappel {

/** @brief A 2D Gaussian with fixed PSF under an Poisson noise assumption and maximum-likelihood objective
 * 
 *   Model: Gauss2DModel - 2D Gaussian PSF with fixed PSF sigma
 *   Objective: PoissonNoise2DObjective - Poisson noise model for 2D
 *   Estimator: MLEstimator - Pure-likelihood estimator
 * 
 */
class Gauss2DMLE : public Gauss2DModel, public PoissonNoise2DObjective, public MLEstimator 
{
public:
    /* Constructor/Destructor */
    Gauss2DMLE(ImageCoordT size, double psf_sigma, const std::string &prior_type = DefaultPriorType);
    Gauss2DMLE(const ImageSizeT &size, double psf_sigma, const std::string &prior_type = DefaultPriorType);
    Gauss2DMLE(const ImageSizeT &size, const VecT &psf_sigma, const std::string &prior_type = DefaultPriorType);
    Gauss2DMLE(const ImageSizeT &size, const VecT &psf_sigma, CompositeDist&& prior);
    Gauss2DMLE(const ImageSizeT &size, const VecT &psf_sigma, const CompositeDist& prior);
    Gauss2DMLE(const Gauss2DMLE &o);
    Gauss2DMLE& operator=(const Gauss2DMLE &o);
    Gauss2DMLE(Gauss2DMLE &&o);
    Gauss2DMLE& operator=(Gauss2DMLE &&o);    
    static const std::string name;
};

} /* namespace mappel */

#endif /* MAPPEL_GAUSS2DMLE_H */
