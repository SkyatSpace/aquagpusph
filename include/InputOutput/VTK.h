/*
 *  This file is part of AQUAgpusph, a free CFD program based on SPH.
 *  Copyright (C) 2012  Jose Luis Cercos Pita <jl.cercos@upm.es>
 *
 *  AQUAgpusph is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  AQUAgpusph is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with AQUAgpusph.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VTK_H_INCLUDED
#define VTK_H_INCLUDED

#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkVertex.h>
#include <vtkCellArray.h>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMDocumentType.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMNodeIterator.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/util/XMLUni.hpp>

#include <sphPrerequisites.h>
#include <InputOutput/Particles.h>

namespace Aqua{
namespace InputOutput{

/** \class VTK VTK.h InputOutput/VTK.h
 * VTK particles data file loader/saver. These files are formatted as binary
 * VTK files.
 * @note The expected fields are:
 *   -# \f$\mathbf{r}$\f.\f$x\f$
 *   -# \f$\mathbf{r}$\f.\f$y\f$
 *   -# \f$\mathbf{r}$\f.\f$z\f$ (For 3d cases only)
 *   -# \f$\mathbf{n}$\f.\f$x\f$
 *   -# \f$\mathbf{n}$\f.\f$y\f$
 *   -# \f$\mathbf{n}$\f.\f$z\f$ (For 3d cases only)
 *   -# \f$\mathbf{v}$\f.\f$x\f$
 *   -# \f$\mathbf{v}$\f.\f$y\f$
 *   -# \f$\mathbf{v}$\f.\f$z\f$ (For 3d cases only)
 *   -# \f$\frac{d \mathbf{v}}{d t}$\f.\f$x\f$
 *   -# \f$\frac{d \mathbf{v}}{d t}$\f.\f$y\f$
 *   -# \f$\frac{d \mathbf{v}}{d t}$\f.\f$z\f$ (For 3d cases only)
 *   -# \f$\rho$\f
 *   -# \f$\frac{d \rho}{d t}$\f
 *   -# \f$m$\f
 *   -# moving flag
 */
class VTK : public Particles
{
public:
	/** Constructor
	 * @param first First particle managed by this saver/loader.
	 * @param n Number of particles managed by this saver/loader.
	 * @param ifluid Fluid index.
	 */
	VTK(unsigned int first, unsigned int n, unsigned int ifluid);

	/** Destructor
	 */
	~VTK();

    /** Save the data. The data
     * @return false if all gone right, true otherwise.
     */
    bool save();

    /** Load the particles data.
     * @return false if all gone right, true otherwise.
     */
    bool load();

private:
    /** Create a new file to write.
     * @return The file handler, NULL if errors happened.
     */
    vtkXMLUnstructuredGridWriter* create();

    /** Create/Update the Paraview Data File.
     * @return false if all gone right, true otherwise.
     */
    bool updatePVD();

    /** Test if the Paraview Data File exist, or create it otherwise.
     * @return The document object.
     */
    xercesc::DOMDocument* getPVD();

    /** PVD file name
     * @return the PVD file name
     */
    const char* filenamePVD();

};  // class InputOutput

}}  // namespaces

#endif // VTK_H_INCLUDED
