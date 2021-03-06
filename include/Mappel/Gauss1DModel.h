/** @file Gauss1DModel.h
 * @author Mark J. Olah (mjo\@cs.unm DOT edu)
 * @date 2014-2019
 * @brief The class declaration and inline and templated functions for Gauss1DModel.
 */

#ifndef MAPPEL_GAUSS1DMODEL_H
#define MAPPEL_GAUSS1DMODEL_H

#include "Mappel/PointEmitterModel.h"
#include "Mappel/ImageFormat1DBase.h"
#include "Mappel/MCMCAdaptor1D.h"

namespace mappel {

    
/** @brief A base class for 1D Gaussian PSF with a fixed sigma (standard dev.)
 *
 * This base class defines the Stencil type for 1D Gaussian PSF as well as the prior shape and parameters.
 * 
 * Initialized by an integer, size, and double, psf_sigma.
 * 
 */
class Gauss1DModel : public virtual PointEmitterModel, public virtual ImageFormat1DBase, public MCMCAdaptor1D
{   
public:
    /** @brief Stencil for 1D fixed-sigma models.
     */
    class Stencil {
    public:
        bool derivatives_computed = false;
        using ParamT = Gauss1DModel::ParamT;
        Gauss1DModel const *model;

        ParamT theta; 
        VecT dx;
        VecT Gx;
        VecT X;
        VecT DX;
        VecT DXS;
        Stencil() {}
        Stencil(const Gauss1DModel &model, const ParamT &theta, bool compute_derivatives=true);
        void compute_derivatives();
        inline double x() const {return theta(0);}
        inline double I() const {return theta(1);}
        inline double bg() const {return theta(2);}
        friend std::ostream& operator<<(std::ostream &out, const Gauss1DModel::Stencil &s);
    };
    using StencilVecT = std::vector<Stencil>;
    
    /* Prior construction */
    static const StringVecT prior_types;
    static const std::string DefaultPriorType;
    static CompositeDist make_default_prior(IdxT size, const std::string &prior_type);
    static CompositeDist make_default_prior_beta_position(IdxT size);
    static CompositeDist make_default_prior_normal_position(IdxT size);
    static CompositeDist make_prior_beta_position(IdxT size, double beta_xpos, double mean_I,
                                               double kappa_I, double mean_bg, double kappa_bg);
    static CompositeDist make_prior_normal_position(IdxT size, double sigma_xpos, double mean_I,
                                               double kappa_I, double mean_bg, double kappa_bg);
    /* psf_sigma accessors */
    double get_psf_sigma() const;
    double get_psf_sigma(IdxT idx) const;
    void set_psf_sigma(double new_psf_sigma);
    void set_psf_sigma(const VecT &new_psf_sigma);

    StatsT get_stats() const;
    Stencil make_stencil(const ParamT &theta, bool compute_derivatives=true) const;

    /* Model Pixel Value And Derivatives */
    double pixel_model_value(IdxT i,  const Stencil &s) const;
    void pixel_grad(IdxT i, const Stencil &s, ParamT &pgrad) const;
    void pixel_grad2(IdxT i, const Stencil &s, ParamT &pgrad2) const;
    void pixel_hess(IdxT i, const Stencil &s, MatT &hess) const;
    void pixel_hess_update(IdxT i, const Stencil &s, double dm_ratio_m1, 
                           double dmm_ratio, ParamT &grad, MatT &hess) const;
                           
    /** @brief Fast, heuristic estimate of initial theta */
    Stencil initial_theta_estimate(const ImageT &im) const;
    Stencil initial_theta_estimate(const ImageT &im, const ParamT &theta_init) const;
protected:
    //Abstract class cannot be instantiated
    Gauss1DModel(IdxT size, double psf_sigma);
    Gauss1DModel(const Gauss1DModel &o);
    Gauss1DModel(Gauss1DModel &&o);
    Gauss1DModel& operator=(const Gauss1DModel &o);
    Gauss1DModel& operator=(Gauss1DModel &&o);
        
    /* Non-static data Members */
    double psf_sigma; /**< Standard deviation of the fixed-sigma 1D Gaussian PSF in pixels */
private:
    static void set_prior_variable_names(CompositeDist &prior);
};


/* Inline Method Definitions */

/** @brief Make a new Model::Stencil object at theta.
 * 
 * Stencils store all of the important calculations necessary for evaluating the log-likelihood and its derivatives at a
 * particular theta (parameter) value.
 * 
 * This allows re-use of the most expensive computations.  Stencils can be easily passed around by reference, and most
 * functions in the mappel::methods namespace accept a const Stencil reference in place of the model parameter.
 * 
 * Throws mappel::ModelBoundsError if not model.theta_in_bounds(theta).
 * 
 * If derivatives will not be computed with this stencil set compute_derivatives=false
 * 
 * @param theta Prameter to evaluate at
 * @param compute_derivatives True to also prepare for derivative computations
 * @return A new Stencil object ready to compute with
 */
inline
Gauss1DModel::Stencil
Gauss1DModel::make_stencil(const ParamT &theta, bool compute_derivatives) const
{
    if(!theta_in_bounds(theta)) {
        std::ostringstream msg;
        msg<<"Theta is not in bounds: "<<theta.t();
        throw ModelBoundsError(msg.str());
    }
    return Stencil(*this,theta,compute_derivatives);
}

inline
double Gauss1DModel::get_psf_sigma() const
{ return psf_sigma; }

inline
void Gauss1DModel::set_psf_sigma(const VecT &new_psf_sigma)
{ set_psf_sigma(new_psf_sigma(0)); }

inline
double Gauss1DModel::pixel_model_value(IdxT i,  const Stencil &s) const
{
    return s.bg()+s.I()*s.X(i);
}

inline
void Gauss1DModel::pixel_grad(IdxT i, const Stencil &s, ParamT &pgrad) const
{
    double I = s.I();
    pgrad(0) = I * s.DX(i);
    pgrad(1) = s.X(i);
    pgrad(2) = 1.;
}

inline
void Gauss1DModel::pixel_grad2(IdxT i, const Stencil &s, ParamT &pgrad2) const
{
    double I = s.I();
    pgrad2(0) = I/psf_sigma * s.DXS(i);
    pgrad2(1) = 0.;
    pgrad2(2) = 0.;
}

inline
void Gauss1DModel::pixel_hess(IdxT i, const Stencil &s, MatT &hess) const
{
    hess.zeros();
    double I = s.I();
    hess(0,0) = I/psf_sigma * s.DXS(i);
    hess(0,1) = s.DX(i); 
}

inline
Gauss1DModel::Stencil 
Gauss1DModel::initial_theta_estimate(const ImageT &im) const
{
    auto theta = make_param();
    theta.zeros();
    return initial_theta_estimate(im,theta);
}

} /* namespace mappel */

#endif /* MAPPEL_GAUSS1DMODEL_H */
