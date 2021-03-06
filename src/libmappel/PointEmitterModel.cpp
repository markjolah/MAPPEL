/** @file PointEmitterModel.cpp
 * @author Mark J. Olah (mjo\@cs.unm DOT edu)
 * @date 2014-2017
 * @brief The class definition and template Specializations for PointEmitterModel
 */
#include <cmath>
#include <algorithm>

#include "Mappel/PointEmitterModel.h"

// #include "Mappel/util.h"
// #include <omp.h>

namespace mappel {
    
const std::string PointEmitterModel::DefaultEstimatorMethod = "TrustRegion"; ///<Default optimization method for MLE/MAP estimation.
const std::string PointEmitterModel::DefaultProfileBoundsEstimatorMethod = "Newton"; ///<Default optimization method for profile bounds optimizations.
const std::string PointEmitterModel::DefaultSeperableInitEstimator = "TrustRegion";
const IdxT PointEmitterModel::DefaultMCMCNumSamples = 300; ///< Number of final samples to use in estimation of posterior properties (mean, credible interval, cov, etc.)
const IdxT PointEmitterModel::DefaultMCMCBurnin = 10; ///< Number of samples to throw away (burn-in) on initialization
const IdxT PointEmitterModel::DefaultMCMCThin = 0; ///< Keep every # samples. [Value of 0 indicates use the model default. This is suggested.]
const double PointEmitterModel::DefaultConfidenceLevel = 0.95; ///< Default level at which to estimate confidence intervals must be in range (0,1).

const double PointEmitterModel::DefaultPriorBetaPos = 3; /**< Default position parameter in symmetric beta-distributions */
const double PointEmitterModel::DefaultPriorSigmaPos = 1; /**< Default position parameter in symmetric beta-distributions */
const double PointEmitterModel::DefaultPriorMeanI = 300; /**< Default emitter intensity mean*/
const double PointEmitterModel::DefaultPriorMaxI = INFINITY; /**< Default maximum emitter intensity*/
const double PointEmitterModel::DefaultPriorIntensityKappa = 2;  /**< Default shape for intensity gamma distributions */
const double PointEmitterModel::DefaultPriorPixelMeanBG = 4; /**< Default per-pixel mean background counts */
const double PointEmitterModel::DefaultPriorPSFSigmaAlpha = 2; /**< Default per-pixel background gamma distribution shape */

const double PointEmitterModel::bounds_epsilon = 1.0E-6; /**< Distance from the boundary to constrain in bound_theta and bounded_theta methods */
const double PointEmitterModel::global_min_psf_sigma = 1E-1; /**< Global minimum for any psf_sigma.  Sizes below this value are invalid, and nowhere near useful for practical point emitter localization */ 
const double PointEmitterModel::global_max_psf_sigma = 1E2; /**< Global maxmimum for any psf_sigma.  Sizes above this value are invalid, and nowhere near useful for practical point emitter localization */ 


/* Constructors and assignment operators */
PointEmitterModel::PointEmitterModel()
    : prior{}
{
    update_cached_prior_values();
}

PointEmitterModel::PointEmitterModel(CompositeDist&& prior_)
    : prior{std::move(prior_)}
{
    update_cached_prior_values();
}

PointEmitterModel::PointEmitterModel(const CompositeDist& prior_)
    : prior{prior_}
{
    update_cached_prior_values();
}

PointEmitterModel::PointEmitterModel(const PointEmitterModel &o) 
    : prior{o.prior}
{
    update_cached_prior_values();
}

PointEmitterModel::PointEmitterModel(PointEmitterModel &&o) 
    : prior{std::move(o.prior)}
{
    update_cached_prior_values();
}

PointEmitterModel& PointEmitterModel::operator=(const PointEmitterModel &o)
{
    prior = o.prior;
    update_cached_prior_values();
    return *this;
}

PointEmitterModel& PointEmitterModel::operator=(PointEmitterModel &&o)
{
    prior = std::move(o.prior);
    update_cached_prior_values();
    return *this;
}

void PointEmitterModel::update_cached_prior_values()
{
    num_params = prior.num_dim();
    num_hyperparams = prior.num_params();
    lbound = prior.lbound();
    ubound = prior.ubound();    
}

/* Static member functions */
prior_hessian::TruncatedNormalDist
PointEmitterModel::make_prior_component_position_normal(IdxT size, double pos_sigma)
{
    double pos_mean = size/2;
    return prior_hessian::make_bounded_normal_dist(pos_mean,pos_sigma,std::make_pair(0.,size));
}
    
prior_hessian::ScaledSymmetricBetaDist
PointEmitterModel::make_prior_component_position_beta(IdxT size, double pos_beta)
{
    return prior_hessian::make_scaled_symmetric_beta_dist(pos_beta,std::make_pair(0.,size));
}

prior_hessian::TruncatedGammaDist
PointEmitterModel::make_prior_component_intensity(double mean, double kappa)
{
    return prior_hessian::make_bounded_gamma_dist(mean/kappa,kappa,std::make_pair(0.,DefaultPriorMaxI));
}

prior_hessian::TruncatedParetoDist
PointEmitterModel::make_prior_component_sigma(double min_sigma, double max_sigma, double alpha)
{
    return prior_hessian::make_bounded_pareto_dist(alpha,std::make_pair(min_sigma,max_sigma));
}

/* Non-static member functions */
void PointEmitterModel::set_rng_seed(RngSeedT seed)
{ 
    rng_manager.seed(seed); 
}

ParallelRngManagerT& PointEmitterModel::get_rng_manager()
{ 
    return rng_manager; 
}

ParallelRngGeneratorT& PointEmitterModel::get_rng_generator()
{ 
    return rng_manager.generator(); 
}

StatsT PointEmitterModel::get_stats() const
{
    StatsT stats;
    stats["num_params"] = num_params;
    stats["num_hyperparams"] = num_hyperparams;
    auto hyperparams = prior.params();
    auto hyperparam_names = prior.param_names();
    std::string hp_str("hyperparameters.");
    for(IdxT i=0; i<num_hyperparams; i++) stats[hp_str+hyperparam_names[i]] = hyperparams[i];
    for(IdxT n=0;n<num_params;n++) {
        std::ostringstream outl,outu;
        outl<<"lbound."<<n+1;
        stats[outl.str()]= lbound(n);
        outu<<"ubound."<<n+1;
        stats[outu.str()]= ubound(n);
    }
    double Dseed;
    uint64_t seed =rng_manager.get_init_seed();
    memcpy(&Dseed, &seed, sizeof(seed));
    stats["prarallelrng.seed"]=Dseed; //Extract bytes in pure form and fix in python
    stats["prarallelrng.num_threads"]=rng_manager.get_num_threads();
    return stats;
}

void PointEmitterModel::set_prior(const CompositeDist& prior_)
{
    prior = prior_;
    num_params = prior.num_dim();
    num_hyperparams = prior.num_dim();
    lbound = prior.lbound();
    ubound = prior.ubound();
}

void PointEmitterModel::set_prior(CompositeDist&& prior_)
{
    prior = std::move(prior_);
    num_params = prior.num_dim();
    num_hyperparams = prior.num_dim();
    lbound = prior.lbound();
    ubound = prior.ubound();
}

void PointEmitterModel::check_param_shape(const ParamT &theta) const
{
    if(theta.n_elem != num_params) {
        std::ostringstream msg;
        msg<<"check_theta: Got bad theta Size= "<<theta.n_elem<<" Expected size="<<num_params;
        throw ArrayShapeError(msg.str());
    }
}

void PointEmitterModel::check_param_shape(const ParamVecT &theta) const
{
    if(theta.n_rows != num_params) {
        std::ostringstream msg;
        msg<<"check_theta: Got bad theta Size= ["<<theta.n_rows<<","<<theta.n_cols<<"] Expected size=["<<num_params<<",...]";
        throw ArrayShapeError(msg.str());
    }
}

void PointEmitterModel::check_psf_sigma(double psf_sigma) const
{
   if(psf_sigma < global_min_psf_sigma || 
      psf_sigma > global_max_psf_sigma || 
      !std::isfinite(psf_sigma)) {
        std::ostringstream msg;
        msg<<"Invalid psf_sigma: "<<psf_sigma<<"\b Valid psf_sigma range:["
            <<global_min_psf_sigma<<","<<global_max_psf_sigma<<"]";
        throw ParameterValueError(msg.str());
    }
}

void PointEmitterModel::check_psf_sigma(const VecT &psf_sigma) const
{
    if(arma::any(psf_sigma < global_min_psf_sigma) || 
        arma::any(psf_sigma > global_max_psf_sigma) || 
        !psf_sigma.is_finite()) {
        std::ostringstream msg;
        msg<<"Invalid psf_sigma: "<<psf_sigma.t()<<"\b Valid psf_sigma range:["
            <<global_min_psf_sigma<<","<<global_max_psf_sigma<<"]";
        throw ParameterValueError(msg.str());
    }
}

/**
 *
 * Modifies the prior bounds to prevent sampling outside the valid box-constraints.
 */
void PointEmitterModel::set_bounds(const ParamT &lbound_, const ParamT &ubound_)
{
    if(lbound_.n_elem != num_params) throw ArraySizeError("Invalid lower bound size");
    if(ubound_.n_elem != num_params) throw ArraySizeError("Invalid upper bound size");
    for(IdxT n=0; n<num_params; n++) {
        if(lbound_(n)>ubound_(n)) throw ParameterValueError("Bounds inverted.");
        if(std::fabs(lbound_(n)-ubound_(n))<10*bounds_epsilon) throw ParameterValueError("Bounds too close.");
    }
    prior.set_bounds(lbound_,ubound_);
    lbound = prior.lbound();
    ubound = prior.ubound();
}

void PointEmitterModel::set_lbound(const ParamT &lbound_)
{
    if(lbound_.n_elem != num_params) throw ArraySizeError("Invalid lower bound size");
    for(IdxT n=0; n<num_params; n++) {
        if(lbound_(n)>ubound(n)) throw ParameterValueError("Bounds inverted.");
        if(std::fabs(lbound_(n)-ubound(n))<10*bounds_epsilon) throw ParameterValueError("Bounds too close.");
    }
    prior.set_lbound(lbound_);
    lbound = prior.lbound();
}

void PointEmitterModel::set_ubound(const ParamT &ubound_)
{
    if(ubound_.n_elem != num_params) throw ArraySizeError("Invalid upper bound size");
    for(IdxT n=0; n<num_params; n++) {
        if(lbound(n)>ubound_(n)) throw ParameterValueError("Bounds inverted.");
        if(std::fabs(lbound(n)-ubound_(n))<10*bounds_epsilon) throw ParameterValueError("Bounds too close.");
    }
    prior.set_ubound(ubound_);
    ubound = prior.ubound();    
}

void PointEmitterModel::bound_theta(ParamT &theta, double epsilon) const
{
    check_param_shape(theta);
    for(IdxT n=0;n<num_params;n++) {
        if(theta(n) <= lbound(n)) theta(n)=lbound(n)+epsilon;
        if(theta(n) >= ubound(n)) theta(n)=ubound(n)-epsilon;
    }
}

bool PointEmitterModel::theta_in_bounds(const ParamT &theta) const
{
    check_param_shape(theta);
    for(IdxT n=0; n<num_params; n++) 
        if(lbound(n) >= theta(n) || theta(n) >= ubound(n)) return false;
    return true;
}

PointEmitterModel::ParamT PointEmitterModel::bounded_theta(const ParamT &theta, double epsilon) const
{
    check_param_shape(theta);
    ParamT btheta = theta;
    for(IdxT n=0;n<num_params;n++) {
        if(theta(n) <= lbound(n)) btheta(n)=lbound(n)+epsilon;
        if(theta(n) >= ubound(n)) btheta(n)=ubound(n)-epsilon;
    }
    return btheta;
}

PointEmitterModel::ParamT PointEmitterModel::reflected_theta(const ParamT &theta) const
{
    check_param_shape(theta);
    ParamT btheta = theta;
    for(IdxT n=0;n<num_params;n++) {
        if(std::isfinite(lbound(n))) {
            if(std::isfinite(ubound(n))){//both valid bounds.  Do reflection
                double d = 2*(ubound(n)-lbound(n));
                double w = std::fmod(std::fabs(theta(n)-lbound(n)), d);
                btheta(n) = std::min(w,d-w)+lbound(n);
            } else if (theta(n)<lbound(n)) {
                btheta(n)=2*lbound(n)-theta(n); //valid lower bound only
            }
        } else if(theta(n)>ubound(n)) {
            btheta(n)=2*ubound(n)-theta(n); //valid upper bound only
        }
    }
    return btheta;
}

BoolVecT PointEmitterModel::theta_stack_in_bounds(const ParamVecT &theta) const
{
    check_param_shape(theta);
    IdxT N = theta.n_cols;
    BoolVecT in_bounds(N);
    for(IdxT n=0; n<N; n++) in_bounds(n) = theta_in_bounds(theta.col(n));
    return in_bounds;
}

PointEmitterModel::ParamVecT 
PointEmitterModel::bounded_theta_stack(const ParamVecT &theta, double epsilon) const
{
    check_param_shape(theta);
    IdxT N = theta.n_cols;
    auto new_theta = make_param_stack(N);
    for(IdxT n=0; n<N; n++) new_theta.col(n) = bounded_theta(theta.col(n),epsilon);
    return new_theta;
}

PointEmitterModel::ParamVecT 
PointEmitterModel::reflected_theta_stack(const ParamVecT &theta) const
{
    check_param_shape(theta);
    IdxT N = theta.n_cols;
    auto new_theta = make_param_stack(N);
    for(IdxT n=0; n<N; n++) new_theta.col(n) = reflected_theta(theta.col(n));
    return new_theta;
}

} /* namespace mappel */
