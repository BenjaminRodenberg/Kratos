// KRATOS___
//     //   ) )
//    //         ___      ___
//   //  ____  //___) ) //   ) )
//  //    / / //       //   / /
// ((____/ / ((____   ((___/ /  MECHANICS
//
//  License:         geo_mechanics_application/license.txt
//
//  Main authors:    Vahid Galavi
//

#if !defined(KRATOS_GEO_CURVED_BEAM_ELEMENT_H_INCLUDED )
#define  KRATOS_GEO_CURVED_BEAM_ELEMENT_H_INCLUDED

// Project includes

// Application includes
#include "custom_elements/geo_structural_base_element.hpp"

namespace Kratos
{
/**
 * @class GeoCurvedBeamElement
 *
 * @brief This is a geometrically non-linear (curved) beam element.
 *        The formulation can be found in papers written by Karan S. Surana, e.g:
 *        "Geometrically non-linear formulation for two dimensional curved beam elements"
 * @author Vahid Galavi
 */

template< unsigned int TDim, unsigned int TNumNodes >
class KRATOS_API(GEO_MECHANICS_APPLICATION) GeoCurvedBeamElement :
    public GeoStructuralBaseElement<TDim,TNumNodes>
{

public:

    typedef std::size_t IndexType;
    typedef Properties PropertiesType;
    typedef Node <3> NodeType;
    typedef Geometry<NodeType> GeometryType;
    typedef Geometry<NodeType>::PointsArrayType NodesArrayType;
    typedef Vector VectorType;
    typedef Matrix MatrixType;
    typedef GeometryData::ShapeFunctionsGradientsType ShapeFunctionsGradientsType;

    /// The definition of the sizetype
    typedef std::size_t SizeType;

    using GeoStructuralBaseElement<TDim,TNumNodes>::mThisIntegrationMethod;
    using GeoStructuralBaseElement<TDim,TNumNodes>::mConstitutiveLawVector;
    using GeoStructuralBaseElement<TDim,TNumNodes>::mStressVector;
    using GeoStructuralBaseElement<TDim,TNumNodes>::N_DOF_ELEMENT;
    using GeoStructuralBaseElement<TDim,TNumNodes>::VoigtSize;
    typedef typename GeoStructuralBaseElement<TDim,TNumNodes>::ElementVariables ElementVariables;

    KRATOS_CLASS_POINTER_DEFINITION( GeoCurvedBeamElement );

///----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    /// Default Constructor
    GeoCurvedBeamElement(IndexType NewId = 0) :
        GeoStructuralBaseElement<TDim,TNumNodes>( NewId ) {}

    /// Constructor using an array of nodes
    GeoCurvedBeamElement(IndexType NewId, const NodesArrayType& ThisNodes) :
        GeoStructuralBaseElement<TDim,TNumNodes>(NewId, ThisNodes) {}

    /// Constructor using Geometry
    GeoCurvedBeamElement(IndexType NewId, GeometryType::Pointer pGeometry) :
        GeoStructuralBaseElement<TDim,TNumNodes>( NewId, pGeometry ) {}

    /// Constructor using Properties
    GeoCurvedBeamElement(IndexType NewId,
                         GeometryType::Pointer pGeometry,
                         PropertiesType::Pointer pProperties) :
        GeoStructuralBaseElement<TDim,TNumNodes>( NewId, pGeometry, pProperties )
        {
            mThisIntegrationMethod = this->GetIntegrationMethod();
        }

    /// Destructor
    virtual ~GeoCurvedBeamElement() {}

///----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    Element::Pointer Create(IndexType NewId,
                            NodesArrayType const& ThisNodes,
                            PropertiesType::Pointer pProperties) const override;

    Element::Pointer Create(IndexType NewId,
                            GeometryType::Pointer pGeom,
                            PropertiesType::Pointer pProperties) const override;

    int Check(const ProcessInfo& rCurrentProcessInfo) const override;

///----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    void CalculateLeftHandSide( MatrixType& rLeftHandSideMatrix,
                                const ProcessInfo& rCurrentProcessInfo ) override;

    void CalculateMassMatrix( MatrixType& rMassMatrix,
                              const ProcessInfo& rCurrentProcessInfo ) override;


///----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

protected:

    /// Member Variables

    SizeType GetIntegrationPointsNumber() const override
    {
        return N_POINT_CROSS * this->GetGeometry().IntegrationPointsNumber(mThisIntegrationMethod);
    }

    double CalculateIntegrationCoefficient( unsigned int GPointCross,
                                            double detJ,
                                            double weight ) const;

    virtual void CalculateBMatrix( Matrix &B,
                                   unsigned int GPointCross,
                                   const BoundedMatrix<double,TDim, TDim> &InvertDetJacobian,
                                   ElementVariables &rVariables ) const;

    void CalculateStrainVector(ElementVariables &rVariables) const;

///----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    virtual void CalculateStiffnessMatrix( MatrixType &rStiffnessMatrix,
                                           const ProcessInfo &CurrentProcessInfo ) override;

    virtual void CalculateAll( MatrixType &rLeftHandSideMatrix,
                               VectorType &rRightHandSideVector,
                               const ProcessInfo &CurrentProcessInfo ) override;

    virtual void CalculateRHS( VectorType &rRightHandSideVector,
                               const ProcessInfo &CurrentProcessInfo ) override;

    virtual void CalculateAndAddLHS(MatrixType &rLeftHandSideMatrix,
                                    ElementVariables &rVariables) const;

    virtual void CalculateAndAddRHS(VectorType &rRightHandSideVector,
                                    ElementVariables &rVariables) const;

    virtual void CalculateTransformationMatrix( Matrix &TransformationMatrix,
                                                const Matrix &GradNpT ) const;

    void CalculateCrossDirection( Matrix &CrossDirection ) const override;

    virtual double CalculateElementAngle(unsigned int GPoint,
                                         const BoundedMatrix<double,TNumNodes,TNumNodes> &DN_DXContainer) const;

    virtual double CalculateElementAngle(const Matrix &GradNpT) const;

    virtual double CalculateElementCrossAngle(unsigned int GPoint,
                                              const BoundedMatrix<double,TNumNodes,TNumNodes> &DN_DXContainer) const;

    virtual void CalculateDeterminantJacobian(unsigned int GPointCross,
                                              const ElementVariables &rVariables,
                                              BoundedMatrix<double,TDim, TDim> &DeterminantJacobian) const;

    virtual void InitializeElementVariables( ElementVariables &rVariables,
                                             ConstitutiveLaw::Parameters &rConstitutiveParameters,
                                             const GeometryType &Geom,
                                             const PropertiesType &Prop,
                                             const ProcessInfo &CurrentProcessInfo ) const override;
    
    void CalculateAndAddBodyForce(VectorType &rRightHandSideVector,
                                  ElementVariables &rVariables) const;

    void CalculateAndAddStiffnessForce(VectorType &rRightHandSideVector, ElementVariables &rVariables) const;
    void SetRotationalInertiaVector(const PropertiesType& Prop, Vector& RotationalInertia) const;

///----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

private:

    /// Serialization

    friend class Serializer;

    void save(Serializer& rSerializer) const override
    {
        KRATOS_SERIALIZE_SAVE_BASE_CLASS( rSerializer, Element )
    }

    void load(Serializer& rSerializer) override
    {
        KRATOS_SERIALIZE_LOAD_BASE_CLASS( rSerializer, Element )
    }

    /// Assignment operator.
    GeoCurvedBeamElement & operator=(GeoCurvedBeamElement const& rOther);

    /// Copy constructor.
    GeoCurvedBeamElement(GeoCurvedBeamElement const& rOther);

    //const values
    static constexpr SizeType N_DOF_NODE_DISP = TDim;
    static constexpr SizeType N_DOF_NODE_ROT = (TDim == 2 ? 1 : 3);
    static constexpr SizeType N_DOF_NODE = N_DOF_NODE_DISP + N_DOF_NODE_ROT;
    static constexpr SizeType N_POINT_CROSS = 2;
    const Vector CrossWeight = UnitVector(N_POINT_CROSS);

}; // Class GeoCurvedBeamElement

} // namespace Kratos

#endif // KRATOS_GEO_CURVED_BEAM_ELEMENT_H_INCLUDED  defined
