
/** @file Gauss1DsMLE.h
 * @author Mark J. Olah (mjo\@cs.unm DOT edu)
 * @date 2014-2019
 * @brief The class declaration and inline and templated functions for Gauss1DsMLE.
 */

#ifndef MAPPEL_GAUSS1DSMLE_H
#define MAPPEL_GAUSS1DSMLE_H

#include "Mappel/Gauss1DsModel.h"
#include "Mappel/PoissonNoise1DObjective.h"
#include "Mappel/MLEstimator.h"
#include "Mappel/model_methods.h" //Declaration of methods

namespace mappel {

/** @brief A 1D Gaussian with variable PSF under an Poisson noise assumption and maximum-likelihood estimator
 * 
 *   Model: Gauss1DsModel - 1D Gaussian PSF with variable PSF sigma
 *   Objective: PoissonNoise1DObjective - Poisson noise model for 1D
 *   Estimator: MLEstimator - Pure-likelihood estimator
 */
class Gauss1DsMLE : public Gauss1DsModel, public PoissonNoise1DObjective, public MLEstimator 
{
public:
    Gauss1DsMLE(arma::Col<ImageCoordT> size, VecT min_sigma, VecT max_sigma, const std::string &prior_type = DefaultPriorType);
    Gauss1DsMLE(ImageSizeT size, double min_sigma, double max_sigma, const std::string &prior_type = DefaultPriorType);
    Gauss1DsMLE(ImageSizeT size, CompositeDist&& prior);
    Gauss1DsMLE(ImageSizeT size, const CompositeDist& prior);     
    Gauss1DsMLE(const Gauss1DsMLE &o);
    Gauss1DsMLE& operator=(const Gauss1DsMLE &o);
    Gauss1DsMLE(Gauss1DsMLE &&o);
    Gauss1DsMLE& operator=(Gauss1DsMLE &&o);    
    static const std::string name;
};
 
} /* namespace mappel */

#endif /* MAPPEL_GAUSS1DSMLE_H */
    
