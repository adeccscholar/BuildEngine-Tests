// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkFeatureEdges.h>
#include <vtkFlyingEdges3D.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMassProperties.h>
#include <vtkNew.h>
#include <vtkPNGWriter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyData.h>
#include <vtkPolyDataConnectivityFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataNormals.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSMPTools.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkVersion.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkWindowToImageFilter.h>
#include <vtkNew.h>
#include <vtkType.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <numbers>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
   struct Options {
      int iResolution { 129 };
      int iIterations { 4 };
      std::string strBackend { "TBB" };
      std::string strScreenshot;
      bool bVerify { false };
      bool bStress { false };
   };

   /**
    \brief Parse deterministic test and visualization switches.
    \details No implicit file access or GPU context is requested by numerical modes.
    \param argc Number of arguments.
    \param argv Argument values.
    \returns Parsed options.
    \throw std::invalid_argument On an invalid switch or range.
    */
   Options ParseOptions(int const argc, char const* const* const argv) {
      Options theOptions;
      for(int i = 1; i < argc; ++i) {
         std::string_view const svArg { argv[i] };
         if(svArg == "--verify") theOptions.bVerify = true;
         else if(svArg == "--stress") theOptions.bStress = true;
         else if(svArg == "--resolution" || svArg == "--iterations" ||
                 svArg == "--backend" || svArg == "--screenshot") {
            if(i + 1 >= argc) throw std::invalid_argument("Missing value for option");
            std::string const strValue { argv[++i] };
            if(svArg == "--resolution") theOptions.iResolution = std::stoi(strValue);
            else if(svArg == "--iterations") theOptions.iIterations = std::stoi(strValue);
            else if(svArg == "--backend") theOptions.strBackend = strValue;
            else theOptions.strScreenshot = strValue;
         }
         else throw std::invalid_argument(std::format("Unknown option: {}", svArg));
      }
      if(theOptions.iResolution < 33 || theOptions.iResolution > 321 ||
         theOptions.iIterations < 1 || theOptions.iIterations > 100)
         throw std::invalid_argument("Resolution must be 33..321, iterations 1..100");
      if(theOptions.strBackend != "TBB" && theOptions.strBackend != "OpenMP" &&
         theOptions.strBackend != "Sequential")
         throw std::invalid_argument("Backend must be TBB, OpenMP or Sequential");
      return theOptions;
   }

   /**
    \brief Generate an analytic sphere field or a smoothly distorted volume.
    \details Scalar storage is written once per voxel by vtkSMPTools; the
             subsequent VTK pipeline owns its data through vtkImageData.
    \param iN Number of points on each axis.
    \param bArtistic Enable smooth deformation for interactive visualization.
    \returns Owned VTK image with floating point scalars.
    \throw std::runtime_error When voxel allocation fails.
    */
   vtkSmartPointer<vtkImageData> MakeField(int const iN, bool const bArtistic) {
      vtkNew<vtkImageData> theImage;
      theImage->SetDimensions(iN, iN, iN);
      double const flSpacing = 2.8 / (iN - 1);
      theImage->SetOrigin(-1.4, -1.4, -1.4);
      theImage->SetSpacing(flSpacing, flSpacing, flSpacing);
      theImage->AllocateScalars(VTK_DOUBLE, 1);
      auto* const pValues = static_cast<double*>(theImage->GetScalarPointer());
      if(!pValues) throw std::runtime_error("VTK scalar allocation failed");
      vtkIdType const iTotal = static_cast<vtkIdType>(iN) * iN * iN;
      vtkSMPTools::For(0, iTotal, [&](vtkIdType const iBegin, vtkIdType const iEnd) {
         for(vtkIdType i = iBegin; i < iEnd; ++i) {
            vtkIdType const iX = i % iN;
            vtkIdType const iY = (i / iN) % iN;
            vtkIdType const iZ = i / (static_cast<vtkIdType>(iN) * iN);
            double const flX = -1.4 + iX * flSpacing;
            double const flY = -1.4 + iY * flSpacing;
            double const flZ = -1.4 + iZ * flSpacing;
            double const flWave = bArtistic
               ? 0.11 * std::sin(6.0 * flX) * std::cos(5.0 * flY) * std::sin(4.0 * flZ)
                  + 0.06 * std::cos(8.0 * flX + 4.0 * flZ)
               : 0.0;
            pValues[i] = flX * flX + flY * flY + flZ * flZ + flWave;
         }
      });
      return theImage.GetPointer();
   }

   struct Statistics {
      vtkIdType iPoints {};
      vtkIdType iTriangles {};
      vtkIdType iBoundaryEdges {};
      int iRegions {};
      double flVolume {};
      double flArea {};
      double flElapsedMs {};
   };

   /**
    \brief Build an iso-surface and validate its topology and mass.
    \details All geometric checks run without opening a graphics context.
    \param theImage Immutable input scalar field.
    \returns Surface metrics for diagnostics and analytical comparison.
    \throw std::runtime_error On empty generated geometry.
    */
   Statistics Measure(vtkImageData* const pImage) {
      auto const tpStart = std::chrono::steady_clock::now();
      vtkNew<vtkFlyingEdges3D> theSurface;
      theSurface->SetInputData(pImage);
      theSurface->SetValue(0, 0.64);
      theSurface->Update();
      vtkPolyData* const pSurface = theSurface->GetOutput();
      if(!pSurface || pSurface->GetNumberOfPolys() == 0)
         throw std::runtime_error("FlyingEdges3D generated an empty surface");
      vtkNew<vtkMassProperties> theMass;
      theMass->SetInputConnection(theSurface->GetOutputPort());
      theMass->Update();
      vtkNew<vtkFeatureEdges> theEdges;
      theEdges->SetInputConnection(theSurface->GetOutputPort());
      theEdges->BoundaryEdgesOn();
      theEdges->NonManifoldEdgesOn();
      theEdges->FeatureEdgesOff();
      theEdges->ManifoldEdgesOff();
      theEdges->Update();
      vtkNew<vtkPolyDataConnectivityFilter> theRegions;
      theRegions->SetInputConnection(theSurface->GetOutputPort());
      theRegions->SetExtractionModeToAllRegions();
      theRegions->Update();
      return {
         .iPoints = pSurface->GetNumberOfPoints(),
         .iTriangles = pSurface->GetNumberOfPolys(),
         .iBoundaryEdges = theEdges->GetOutput()->GetNumberOfCells(),
         .iRegions = theRegions->GetNumberOfExtractedRegions(),
         .flVolume = theMass->GetVolume(),
         .flArea = theMass->GetSurfaceArea(),
         .flElapsedMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - tpStart).count()
      };
   }

   /**
    \brief Print one deterministic benchmark record.
    \details CSV-like records can be compared across Release and Debug runs.
    \param theStatistics Computed metrics.
    \param strBackend Selected VTK parallel backend.
    \param iN Cubic grid resolution.
    \param iIteration Measurement iteration number.
    \returns Nothing.
    \throw None.
    */
   void PrintMetrics(Statistics const& theStatistics, std::string const& strBackend,
                     int const iN, int const iIteration) {
      std::println("VTK|METRIC|backend={}|n={}|run={}|points={}|triangles={}|boundary={}|regions={}|volume={:.7f}|area={:.7f}|ms={:.3f}",
         strBackend, iN, iIteration,
         theStatistics.iPoints, theStatistics.iTriangles,
         theStatistics.iBoundaryEdges, theStatistics.iRegions,
         theStatistics.flVolume, theStatistics.flArea, theStatistics.flElapsedMs);
   }

   /**
    \brief Validate analytical sphere metrics within sampling tolerances.
    \details Uses the exact sphere radius 0.8 from the iso-value 0.64.
    \param theStatistics Generated mesh metrics.
    \returns True if geometry, topology and numerical accuracy pass.
    \throw None.
    */
   bool VerifySphere(Statistics const& theStatistics) {
      double constexpr flRadius = 0.8;
      double const flExpectedVolume = 4.0 / 3.0 * std::numbers::pi * std::pow(flRadius, 3);
      double const flExpectedArea = 4.0 * std::numbers::pi * flRadius * flRadius;
      double const flVolumeError = std::abs(theStatistics.flVolume - flExpectedVolume) / flExpectedVolume;
      double const flAreaError = std::abs(theStatistics.flArea - flExpectedArea) / flExpectedArea;
      bool const bTopology = theStatistics.iBoundaryEdges == 0 && theStatistics.iRegions == 1;
      bool const bNumerical = flVolumeError < 0.035 && flAreaError < 0.035;
      bool const bValid = bTopology && bNumerical && theStatistics.iTriangles > 500;
      std::println("VTK|CHECK|topology|{}|regions={} boundary_edges={}",
         bTopology ? "PASS" : "FAIL", theStatistics.iRegions, theStatistics.iBoundaryEdges);
      std::println("VTK|CHECK|geometry|{}|volume_error={:.5f} area_error={:.5f}",
         bNumerical ? "PASS" : "FAIL", flVolumeError, flAreaError);
      return bValid;
   }

   /**
    \brief Construct an interactive dual-viewport scientific visualization.
    \details Native Win32 OpenGL renderer with surface and GPU volume raycast.
    \param theOptions Runtime options and optional PNG output location.
    \returns Zero on success, nonzero on any renderer or image validation error.
    \throw std::exception On VTK or filesystem failure.
    */
   int Visualize(Options const& theOptions) {
      auto const theImage = MakeField(theOptions.iResolution, true);

      vtkNew<vtkFlyingEdges3D> theContour;
      theContour->SetInputData(theImage);
      theContour->SetValue(0, 0.64);
      theContour->Update();
      vtkNew<vtkPolyDataNormals> theNormals;
      theNormals->SetInputConnection(theContour->GetOutputPort());
      theNormals->ConsistencyOn();
      theNormals->AutoOrientNormalsOn();
      theNormals->Update();

      vtkNew<vtkPolyDataMapper> theSurfaceMapper;
      theSurfaceMapper->SetInputConnection(theNormals->GetOutputPort());
      theSurfaceMapper->ScalarVisibilityOff();
      vtkNew<vtkActor> theSurfaceActor;
      theSurfaceActor->SetMapper(theSurfaceMapper);
      theSurfaceActor->GetProperty()->SetColor(0.16, 0.75, 0.95);
      theSurfaceActor->GetProperty()->SetSpecular(0.7);
      theSurfaceActor->GetProperty()->SetSpecularPower(55);

      vtkNew<vtkPolyDataMapper> theWireMapper;
      theWireMapper->SetInputConnection(theNormals->GetOutputPort());
      theWireMapper->ScalarVisibilityOff();
      vtkNew<vtkActor> theWireActor;
      theWireActor->SetMapper(theWireMapper);
      theWireActor->GetProperty()->SetRepresentationToWireframe();
      theWireActor->GetProperty()->SetColor(0.55, 0.94, 1.0);
      theWireActor->GetProperty()->SetOpacity(0.13);
      theWireActor->GetProperty()->SetLineWidth(1.0);

      vtkNew<vtkColorTransferFunction> theColors;
      theColors->AddRGBPoint(0.00, 0.98, 0.40, 0.22);
      theColors->AddRGBPoint(0.32, 0.86, 0.18, 0.73);
      theColors->AddRGBPoint(0.64, 0.23, 0.75, 0.99);
      theColors->AddRGBPoint(1.60, 0.05, 0.04, 0.15);
      vtkNew<vtkPiecewiseFunction> theOpacity;
      theOpacity->AddPoint(0.00, 0.35);
      theOpacity->AddPoint(0.34, 0.22);
      theOpacity->AddPoint(0.66, 0.10);
      theOpacity->AddPoint(1.20, 0.00);
      theOpacity->AddPoint(6.00, 0.00);
      vtkNew<vtkVolumeProperty> theVolumeProperty;
      theVolumeProperty->SetColor(theColors);
      theVolumeProperty->SetScalarOpacity(theOpacity);
      theVolumeProperty->SetInterpolationTypeToLinear();
      theVolumeProperty->ShadeOn();
      theVolumeProperty->SetAmbient(0.25);
      theVolumeProperty->SetDiffuse(0.65);
      theVolumeProperty->SetSpecular(0.3);

      vtkNew<vtkGPUVolumeRayCastMapper> theRayCaster;
      theRayCaster->SetInputData(theImage);
      vtkNew<vtkVolume> theVolume;
      theVolume->SetMapper(theRayCaster);
      theVolume->SetProperty(theVolumeProperty);

      vtkNew<vtkRenderer> theLeft;
      theLeft->SetViewport(0.0, 0.0, 0.5, 1.0);
      theLeft->SetBackground(0.025, 0.035, 0.075);
      theLeft->SetBackground2(0.07, 0.11, 0.20);
      theLeft->GradientBackgroundOn();
      theLeft->AddActor(theSurfaceActor);
      theLeft->AddActor(theWireActor);

      vtkNew<vtkRenderer> theRight;
      theRight->SetViewport(0.5, 0.0, 1.0, 1.0);
      theRight->SetBackground(0.025, 0.035, 0.075);
      theRight->SetBackground2(0.11, 0.04, 0.14);
      theRight->GradientBackgroundOn();
      theRight->AddVolume(theVolume);

      auto addCaption=[](vtkRenderer* const pRenderer, char const* const szTitle,
                         double const flR, double const flG, double const flB) {
         vtkNew<vtkTextActor> theCaption;
         theCaption->SetInput(szTitle);
         theCaption->SetPosition(24, 25);
         theCaption->GetTextProperty()->SetFontSize(23);
         theCaption->GetTextProperty()->SetBold(1);
         theCaption->GetTextProperty()->SetColor(flR, flG, flB);
         pRenderer->AddViewProp(theCaption);
      };
      addCaption(theLeft, "VTK 9.7.2   |   ISOSURFACE / WIREFRAME", 0.62, 0.90, 1.0);
      addCaption(theRight, "VOLUME / GPU RAYCAST", 1.0, 0.58, 0.85);
      theLeft->ResetCamera();
      theRight->ResetCamera();
      for(auto* const pCamera : {theLeft->GetActiveCamera(), theRight->GetActiveCamera()}) {
         pCamera->SetPosition(3.2, 2.5, 3.1);
         pCamera->SetFocalPoint(0, 0, 0);
         pCamera->SetViewUp(0, 0, 1);
         pCamera->SetClippingRange(0.1, 15.0);
         pCamera->Zoom(1.12);
      }

      vtkNew<vtkRenderWindow> theWindow;
      theWindow->SetWindowName("adecc Scholar  |  VTK Scientific Visualization Laboratory");
      theWindow->SetSize(1600, 900);
      theWindow->AddRenderer(theLeft);
      theWindow->AddRenderer(theRight);
      vtkNew<vtkRenderWindowInteractor> theInteractor;
      vtkNew<vtkInteractorStyleTrackballCamera> theStyle;
      theInteractor->SetRenderWindow(theWindow);
      theInteractor->SetInteractorStyle(theStyle);
      theWindow->Render();

      if(!theOptions.strScreenshot.empty()) {
         std::filesystem::path const thePath {theOptions.strScreenshot};
         if(thePath.has_parent_path()) std::filesystem::create_directories(thePath.parent_path());
         vtkNew<vtkWindowToImageFilter> theCapture;
         theCapture->SetInput(theWindow);
         theCapture->SetInputBufferTypeToRGB();
         theCapture->ReadFrontBufferOff();
         theCapture->Update();
         vtkNew<vtkPNGWriter> theWriter;
         theWriter->SetInputConnection(theCapture->GetOutputPort());
         theWriter->SetFileName(thePath.string().c_str());
         theWriter->Write();
         if(!std::filesystem::is_regular_file(thePath) ||
            std::filesystem::file_size(thePath) < 4096)
            throw std::runtime_error("VTK screenshot not written or unexpectedly small");
         std::println("VTK|CHECK|screenshot|PASS|{}", thePath.string());
         return 0;
      }

      std::println("VTK|READY|interactive|backend={} grid={}^3", vtkSMPTools::GetBackend(), theOptions.iResolution);
      theInteractor->Initialize();
      theInteractor->Start();
      return 0;
   }
}

int main(int const argc, char const* const* const argv) {
   try {
      Options const theOptions = ParseOptions(argc, argv);
      if(!vtkSMPTools::SetBackend(theOptions.strBackend.c_str())) {
         std::println(stderr, "VTK|RESULT|FAIL|SMP backend {} unavailable", theOptions.strBackend);
         return 2;
      }
      std::println("VTK|INFO|version={}|backend={}|resolution={}", vtkVersion::GetVTKVersion(),
         vtkSMPTools::GetBackend(), theOptions.iResolution);
      if(theOptions.bVerify || theOptions.bStress) {
         bool bValid = true;
         for(int i = 0; i < (theOptions.bStress ? theOptions.iIterations : 1); ++i) {
            auto const theImage = MakeField(theOptions.iResolution, false);
            Statistics const theStatistics = Measure(theImage);
            PrintMetrics(theStatistics, theOptions.strBackend, theOptions.iResolution, i + 1);
            bValid = VerifySphere(theStatistics) && bValid;
         }
         std::println("VTK|RESULT|{}|analytic sphere / SMP geometry", bValid ? "PASS" : "FAIL");
         return bValid ? 0 : 1;
      }
      return Visualize(theOptions);
   }
   catch(std::exception const& theError) {
      std::println(stderr, "VTK|RESULT|FAIL|{}", theError.what());
      return 2;
   }
}
