//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:         BSD License
//                   Kratos default license: kratos/license.txt
//
//


#if !defined(KRATOS_STRUCTURE_MPM_MODELER_H_INCLUDED )
#define  KRATOS_STRUCTURE_MPM_MODELER_H_INCLUDED


// System includes

// External includes

// Project includes
#include "modeler/modeler.h"

#include "geometries/geometry_data.h"
#include "geometries/coupling_geometry.h"
#include "geometries/line_2d_2.h"
#include "utilities/quadrature_points_utility.h"
#include "utilities/binbased_fast_point_locator.h"
#include "particle_mechanics_application_variables.h"

namespace Kratos
{

///@name Kratos Classes
///@{

/// Finds realtion between structure FEM and MPM to make Mortar Mapping.
/* Detail class definition.
*/
class KRATOS_API(PARTICLE_MECHANICS_APPLICATION) StructureMpmModeler
    : public Modeler
{
public:
    ///@name Type Definitions
    ///@{

    /// Pointer definition of Modeler
    KRATOS_CLASS_POINTER_DEFINITION(StructureMpmModeler);

    typedef std::size_t SizeType;
    typedef std::size_t IndexType;

    typedef Node<3> NodeType;
    typedef Geometry<NodeType> GeometryType;
    typedef typename GeometryType::Pointer GeometryPointerType;

    ///@}
    ///@name Life Cycle
    ///@{

    /// Default constructor.
    StructureMpmModeler()
        : Modeler()
    {
        mpModelOrigin = nullptr;
        mpModelDest = nullptr;
    }

    /// Constructor.
    StructureMpmModeler(
        Model& rModel,
        Parameters ModelerParameters = Parameters())
        : Modeler(rModel, ModelerParameters)
    {
        mpModelOrigin = &rModel;
        if (mpModelOrigin->HasModelPart("Background_Grid")) mIsOriginMpm = true;
        else mIsOriginMpm = false;

        mpModelDest = nullptr;
    }

    /// Destructor.
    virtual ~StructureMpmModeler() = default;

    /// Creates the Modeler Pointer
    Modeler::Pointer Create(
        Model& rModel, const Parameters ModelParameters) const override
    {
        return Kratos::make_shared<StructureMpmModeler>(rModel, ModelParameters);
    }

    /// Adds the second model part to the modeler.
    void GenerateNodes(ModelPart& ThisModelPart) override
    {
        mpModelDest = &ThisModelPart.GetModel();
    }

    ///@}
    ///@name Stages
    ///@{

    void SetupGeometryModel() override;

    void UpdateGeometryModel();

    void PrepareGeometryModel() override
    {
        this->UpdateGeometryModel();
    }

    ///@}
    ///@name Model Part generation
    ///@{

    template<
        class TLineGeometriesList,
        class TQuadraturePointGeometriesList = TLineGeometriesList>
    void CreateStructureQuadraturePointGeometries(
        TLineGeometriesList& rInputLineGeometries,
        TQuadraturePointGeometriesList& rOuputQuadraturePointGeometries,
        GeometryData::IntegrationMethod ThisIntegrationMethod = GeometryData::IntegrationMethod::GI_GAUSS_5
        )
    {
        for (IndexType i = 0; i < rInputLineGeometries.size(); ++i)
        {
            std::vector<GeometryPointerType> qudrature_point_geometries = CreateQuadraturePointsUtility<Node<3>>::Create(rInputLineGeometries[i], ThisIntegrationMethod);
            for (IndexType i = 0; i < qudrature_point_geometries.size(); ++i) {
                rOuputQuadraturePointGeometries.push_back(qudrature_point_geometries[i]);
            }
        }

        double fem_edge_length = 0.0;
        double fem_edge_length_combined = 0.0;
        Vector jac(1);
        for (size_t i = 0; i < rOuputQuadraturePointGeometries.size(); i++)
        {
            rOuputQuadraturePointGeometries[i]->DeterminantOfJacobian(jac);
            fem_edge_length += jac[0];
            //KRATOS_WATCH(rOuputQuadraturePointGeometries[i]->DeterminantOfJacobian(jac))
            //KRATOS_WATCH(rOuputQuadraturePointGeometries[i]->IntegrationPoints()[0].Weight());
            fem_edge_length_combined += (jac[0] * rOuputQuadraturePointGeometries[i]->IntegrationPoints()[0].Weight());
        }

        KRATOS_WATCH(fem_edge_length_combined)
        //KRATOS_WATCH(fem_edge_length)
    }

    template<SizeType TDimension, class TQuadraturePointGeometriesList>
    void CreateMpmQuadraturePointGeometries(
        const TQuadraturePointGeometriesList& rInputQuadraturePointGeometries,
        TQuadraturePointGeometriesList& rOuputQuadraturePointGeometries,
        ModelPart& rBackgroundGridModelPart) {
        if (rOuputQuadraturePointGeometries.size() != rInputQuadraturePointGeometries.size()) {
            rOuputQuadraturePointGeometries.resize(rInputQuadraturePointGeometries.size());
        }

        BinBasedFastPointLocator<TDimension> SearchStructure(rBackgroundGridModelPart);
        SearchStructure.UpdateSearchDatabase();
        typename BinBasedFastPointLocator<TDimension>::ResultContainerType results(100);
        double mpm_edge_length = 0;
        // Loop over the submodelpart of rInitialModelPart
        for (IndexType i = 0; i < rInputQuadraturePointGeometries.size(); ++i)
        {
            typename BinBasedFastPointLocator<TDimension>::ResultIteratorType result_begin = results.begin();

            array_1d<double, 3> coordinates = rInputQuadraturePointGeometries[i]->Center();

            Element::Pointer p_elem;
            Vector N;

            // FindPointOnMesh find the background element in which a given point falls and the relative shape functions
            bool is_found = SearchStructure.FindPointOnMesh(coordinates, N, p_elem, result_begin,100,1e-12);

            if (is_found) {
                const double integration_weight = rInputQuadraturePointGeometries[i]->IntegrationPoints()[0].Weight();

                array_1d<double, 3> local_coordinates;
                auto& r_geometry = p_elem->GetGeometry();
                r_geometry.PointLocalCoordinates(local_coordinates, coordinates);

                IntegrationPoint<3> int_p(local_coordinates, integration_weight);
                Vector N;
                r_geometry.ShapeFunctionsValues(N, local_coordinates);

                Matrix DN_De;
                r_geometry.ShapeFunctionsLocalGradients(DN_De, local_coordinates);
                Matrix DN_De_non_zero(DN_De.size1(), DN_De.size2());

                typename GeometryType::PointsArrayType points;

                Matrix N_matrix(1, N.size());
                SizeType non_zero_counter = 0;
                for (IndexType i_N = 0; i_N < N.size(); ++i_N) {
                    if (N[i_N] > 1e-10) {
                        N_matrix(0, non_zero_counter) = N[i_N];
                        for (IndexType j = 0; j < DN_De.size2(); j++) {
                            DN_De_non_zero(non_zero_counter, j) = DN_De(i_N, j);
                        }
                        points.push_back(r_geometry(i_N));
                        non_zero_counter++;
                    }
                }

                N_matrix.resize(1, non_zero_counter, true);
                DN_De_non_zero.resize(non_zero_counter, DN_De.size2(), true);

                GeometryShapeFunctionContainer<GeometryData::IntegrationMethod> data_container(
                    r_geometry.GetDefaultIntegrationMethod(),
                    int_p,
                    N_matrix,
                    DN_De_non_zero);

                rOuputQuadraturePointGeometries[i] = CreateQuadraturePointsUtility<NodeType>::CreateQuadraturePoint(
                    r_geometry.WorkingSpaceDimension(),
                    r_geometry.LocalSpaceDimension(),
                    data_container,
                    points,
                    &r_geometry);

                Vector jac(1);
                if (!mIsOriginMpm)
                {
                    // we need to set the jacobian of the mpm quad points to the interface jacobian
                    rInputQuadraturePointGeometries[i]->DeterminantOfJacobian(jac);
                    //KRATOS_WATCH(jac[0]);
                    KRATOS_WATCH(rInputQuadraturePointGeometries[i]->IntegrationPoints()[0].Weight());
                    KRATOS_WATCH(rOuputQuadraturePointGeometries[i]->IntegrationPoints()[0].Weight());
                    double combined_weight = jac[0] * rInputQuadraturePointGeometries[i]->IntegrationPoints()[0].Weight();
                    rOuputQuadraturePointGeometries[i]->SetValue(INTEGRATION_WEIGHT, jac[0]);
                    //rOuputQuadraturePointGeometries[i]->SetValue(INTEGRATION_WEIGHT, combined_weight);

                }
                rOuputQuadraturePointGeometries[i]->DeterminantOfJacobian(jac);
                mpm_edge_length += jac[0];
            }
        }

        KRATOS_WATCH(mpm_edge_length)
    }

    template<SizeType TDimension,
        class TConditionsList>
    void UpdateMpmQuadraturePointGeometries(
        TConditionsList& rInputConditions,
        ModelPart& rBackgroundGridModelPart) {
        BinBasedFastPointLocator<TDimension> SearchStructure(rBackgroundGridModelPart);
        SearchStructure.UpdateSearchDatabase();
        typename BinBasedFastPointLocator<TDimension>::ResultContainerType results(100);

        const IndexType mpm_index = (mIsOriginMpm) ? 0 : 1;
        const IndexType fem_index = 1- mpm_index;

        Geometry<Node<3>>* p_quad_geom;
        auto cond_begin = rInputConditions.ptr_begin();

        // Loop over the submodelpart of rInitialModelPart
        for (IndexType i = 0; i < rInputConditions.size(); ++i)
        {
            typename BinBasedFastPointLocator<TDimension>::ResultIteratorType result_begin = results.begin();

            p_quad_geom = (&((*(cond_begin+i))->GetGeometry()));

            array_1d<double, 3> coordinates = p_quad_geom->GetGeometryPart(fem_index).Center();

            Element::Pointer p_elem;
            Vector N;

            // FindPointOnMesh find the background element in which a given point falls and the relative shape functions
            bool is_found = SearchStructure.FindPointOnMesh(coordinates, N, p_elem, result_begin,100,1e-12);
            array_1d<double, 3> local_coordinates;
            p_elem->GetGeometry().PointLocalCoordinates(local_coordinates, coordinates);

            if (is_found) {
                CreateQuadraturePointsUtility<NodeType>::UpdateFromLocalCoordinates(
                    p_quad_geom->pGetGeometryPart(mpm_index),
                    local_coordinates,
                    p_quad_geom->IntegrationPoints()[0].Weight(),
                    p_elem->GetGeometry());
            }
        }
    }

    ///@}
    ///@name Input and output
    ///@{

    /// Turn back information as a string.
    std::string Info() const override
    {
        return "StructureMpmModeler";
    }

    /// Print information about this object.
    void PrintInfo(std::ostream & rOStream) const override
    {
        rOStream << Info();
    }

    /// Print object's data.
    void PrintData(std::ostream & rOStream) const override
    {
    }

    ///@}

private:
    Model* mpModelOrigin;

    Model* mpModelDest;

    bool mIsOriginMpm;

    void CopySubModelPart(ModelPart& rDestinationMP, ModelPart& rReferenceMP)
    {
        rDestinationMP.SetNodes(rReferenceMP.pNodes());
        ModelPart& coupling_conditions = rReferenceMP.GetSubModelPart("coupling_conditions");
        rDestinationMP.SetConditions(coupling_conditions.pConditions());
    }

    void CreateInterfaceLineCouplingConditions(ModelPart& rInterfaceModelPart,
        std::vector<GeometryPointerType>& rGeometries);

    void CheckParameters();

    void FixMPMDestInterfaceNodes(ModelPart& rMPMDestInterfaceModelPart)
    {
        if (!mIsOriginMpm)
        {
            for (size_t i = 0; i < rMPMDestInterfaceModelPart.NumberOfNodes(); i++)
            {
                rMPMDestInterfaceModelPart.NodesArray()[i]->Fix(DISPLACEMENT_X);
                rMPMDestInterfaceModelPart.NodesArray()[i]->Fix(DISPLACEMENT_Y);
                rMPMDestInterfaceModelPart.NodesArray()[i]->Fix(DISPLACEMENT_Z);
            }
        }
    }

    void ReleaseMPMDestInterfaceNodes(ModelPart& rMPMDestInterfaceModelPart)
    {
        if (!mIsOriginMpm)
        {
            for (size_t i = 0; i < rMPMDestInterfaceModelPart.NumberOfNodes(); i++)
            {
                rMPMDestInterfaceModelPart.NodesArray()[i]->Free(DISPLACEMENT_X);
                rMPMDestInterfaceModelPart.NodesArray()[i]->Free(DISPLACEMENT_Y);
                rMPMDestInterfaceModelPart.NodesArray()[i]->Free(DISPLACEMENT_Z);
            }
        }
    }

    ///@}
    ///@name Serializer
    ///@{

    friend class Serializer;

    ///@}
}; // Class StructureMpmModeler

///@}
///@name Input and output
///@{

/// input stream function
inline std::istream& operator >> (
    std::istream& rIStream,
    StructureMpmModeler& rThis);

/// output stream function
inline std::ostream& operator << (
    std::ostream& rOStream,
    const StructureMpmModeler& rThis)
{
    rThis.PrintInfo(rOStream);
    rOStream << std::endl;
    rThis.PrintData(rOStream);

    return rOStream;
}

///@}

}  // namespace Kratos.

#endif // KRATOS_STRUCTURE_MPM_MODELER_H_INCLUDED  defined
