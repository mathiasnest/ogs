/**
 * \file
 * \copyright
 * Copyright (c) 2012-2017, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 *
 * Implementation of Minkley Model
 *
 * Refer to ... 
 * for more details for the tests.
 */

#pragma once

#ifndef NDEBUG
#include <ostream>
#endif

#include "BaseLib/Error.h"
#include "NumLib/NewtonRaphson.h"
#include "ProcessLib/Parameter/Parameter.h"

#include "KelvinVector.h"
#include "MechanicsBase.h"

namespace MaterialLib
{
namespace Solids
{
namespace Minkley
{
/// material parameters 
struct MaterialPropertiesParameters
{
    using P = ProcessLib::Parameter<double>;

    MaterialPropertiesParameters(P const& G_, P const& K_, P const& alpha_,
                                 P const& beta_, P const& gamma_,
                                 P const& delta_, P const& epsilon_,
                                 P const& m_, P const& alpha_p_,
                                 P const& beta_p_, P const& gamma_p_,
                                 P const& delta_p_, P const& epsilon_p_,
                                 P const& m_p_, P const& kappa_,
                                 P const& hardening_coefficient_)
        : G(G_),
          K(K_),
          alpha(alpha_),
          beta(beta_),
          gamma(gamma_),
          delta(delta_),
          epsilon(epsilon_),
          m(m_),
          alpha_p(alpha_p_),
          beta_p(beta_p_),
          gamma_p(gamma_p_),
          delta_p(delta_p_),
          epsilon_p(epsilon_p_),
          m_p(m_p_),
          kappa(kappa_),
          hardening_coefficient(hardening_coefficient_)
    {
    }
    // basic material parameters
    P const& G;  ///< shear modulus
    P const& K;  ///< bulk modulus

    P const& alpha;    ///< material dependent parameter 
    P const& beta;     ///< \copydoc alpha
    P const& gamma;    ///< \copydoc alpha
    P const& delta;    ///< \copydoc alpha
    P const& epsilon;  ///< \copydoc alpha
    P const& m;        ///< \copydoc alpha

    P const& alpha_p;    ///< \copydoc alpha
    P const& beta_p;     ///< \copydoc alpha
    P const& gamma_p;    ///< \copydoc alpha
    P const& delta_p;    ///< \copydoc alpha
    P const& epsilon_p;  ///< \copydoc alpha
    P const& m_p;        ///< \copydoc alpha

    P const& kappa;  ///< hardening parameter
    P const& hardening_coefficient;
};

struct DamagePropertiesParameters
{
    using P = ProcessLib::Parameter<double>;
    P const& alpha_d;
    P const& beta_d;
    P const& h_d;
};

template <typename KelvinVector>
struct PlasticStrain final
{
    PlasticStrain() : D(KelvinVector::Zero()) {}
    PlasticStrain(KelvinVector eps_p_D_, double const eps_p_V_,
                  double const eps_p_eff_)
        : D(std::move(eps_p_D_)), V(eps_p_V_), eff(eps_p_eff_){};

    KelvinVector D;  ///< deviatoric plastic strain
    double V = 0;    ///< volumetric strain
    double eff = 0;  ///< effective plastic strain
};

class Damage final
{
public:
    Damage() = default;
    Damage(double const kappa_d, double const value)
        : _kappa_d(kappa_d), _value(value)
    {
    }

    double kappa_d() const { return _kappa_d; }
    double value() const { return _value; }

private:
    double _kappa_d = 0;  ///< damage driving variable
    double _value = 0;    ///< isotropic damage variable
};

template <int DisplacementDim>
struct StateVariables
    : public MechanicsBase<DisplacementDim>::MaterialStateVariables
{
    StateVariables& operator=(StateVariables const&) = default;
    typename MechanicsBase<DisplacementDim>::MaterialStateVariables& operator=(
        typename MechanicsBase<DisplacementDim>::MaterialStateVariables const&
            state) noexcept override
    {
        assert(dynamic_cast<StateVariables const*>(&state) != nullptr);
        return operator=(static_cast<StateVariables const&>(state));
    }

    void setInitialConditions()
    {
        eps_p = eps_p_prev;
        damage = damage_prev;
    }

    void pushBackState() override
    {
        eps_p_prev = eps_p;
        damage_prev = damage;
    }

    using KelvinVector = ProcessLib::KelvinVectorType<DisplacementDim>;

    PlasticStrain<KelvinVector> eps_p;  ///< plastic part of the state.
    Damage damage;                      ///< damage part of the state.

    // Initial values from previous timestep
    PlasticStrain<KelvinVector> eps_p_prev;  ///< \copydoc eps_p
    Damage damage_prev;                      ///< \copydoc damage

#ifndef NDEBUG
    friend std::ostream& operator<<(
        std::ostream& os, StateVariables<DisplacementDim> const& m)
    {
        os << "State:\n"
           << "eps_p_D: " << m.eps_p.D << "\n"
           << "eps_p_eff: " << m.eps_p.eff << "\n"
           << "kappa_d: " << m.damage.kappa_d() << "\n"
           << "damage: " << m.damage.value() << "\n"
           << "eps_p_D_prev: " << m.eps_p_prev.D << "\n"
           << "eps_p_eff_prev: " << m.eps_p_prev.eff << "\n"
           << "kappa_d_prev: " << m.damage_prev.kappa_d() << "\n"
           << "damage_prev: " << m.damage_prev.value() << "\n"
           << "lambda: " << m.lambda << "\n";
        return os;
    }
#endif  // NDEBUG
};

template <int DisplacementDim>
class SolidMinkley final : public MechanicsBase<DisplacementDim>
{
public:
    static int const KelvinVectorSize =
        ProcessLib::KelvinVectorDimensions<DisplacementDim>::value;
    static int const JacobianResidualSize =
        2 * KelvinVectorSize + 3;  // 2 is the number of components in the
                                   // jacobian/residual, not the space
                                   // dimension. And 3 is for additional
                                   // variables.
    using ResidualVectorType = Eigen::Matrix<double, JacobianResidualSize, 1>;
    using JacobianMatrix = Eigen::Matrix<double, JacobianResidualSize,
                                         JacobianResidualSize, Eigen::RowMajor>;

public:
    std::unique_ptr<
        typename MechanicsBase<DisplacementDim>::MaterialStateVariables>
    createMaterialStateVariables() override
    {
        return std::make_unique<StateVariables<DisplacementDim>>();
    }

public:
    using KelvinVector = ProcessLib::KelvinVectorType<DisplacementDim>;
    using KelvinMatrix = ProcessLib::KelvinMatrixType<DisplacementDim>;

public:
    explicit SolidMinkley(
        NumLib::NewtonRaphsonSolverParameters nonlinear_solver_parameters,
        MaterialPropertiesParameters material_properties,
        std::unique_ptr<DamagePropertiesParameters>&& damage_properties)
        : _nonlinear_solver_parameters(std::move(nonlinear_solver_parameters)),
          _mp(std::move(material_properties)),
          _damage_properties(std::move(damage_properties))
    {
    }

    boost::optional<
        std::tuple<KelvinVector, std::unique_ptr<typename MechanicsBase<
                                     DisplacementDim>::MaterialStateVariables>,
                   KelvinMatrix>>
    integrateStress(
        double const t,
        ProcessLib::SpatialPosition const& x,
        double const dt,
        KelvinVector const& eps_prev,
        KelvinVector const& eps,
        KelvinVector const& sigma_prev,
        typename MechanicsBase<DisplacementDim>::MaterialStateVariables const&
            material_state_variables) override;

private:
    NumLib::NewtonRaphsonSolverParameters const _nonlinear_solver_parameters;

    MaterialPropertiesParameters _mp;
    std::unique_ptr<DamagePropertiesParameters> _damage_properties;
};

extern template class SolidMinkley<2>;
extern template class SolidMinkley<3>;
}  // namespace Minkley
}  // namespace Solids
}  // namespace MaterialLib