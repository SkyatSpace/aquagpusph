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

/** @file
 * @brief Input and output files managing.
 * (See Aqua::InputOutput::FileManager for details)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FileManager.h>
#include <ScreenManager.h>
#include <ProblemSetup.h>
#include <InputOutput/ASCII.h>
#include <InputOutput/FastASCII.h>
#ifdef HAVE_VTK
    #include <InputOutput/VTK.h>
#endif // HAVE_VTK

namespace Aqua{ namespace InputOutput{

FileManager::FileManager()
    : _state()
    , _log()
    , _simulation()
    , _in_file("Input.xml")
{
}

FileManager::~FileManager()
{
    unsigned int i;

    for(auto i = _loaders.begin(); i != _loaders.end(); i++) {
        delete *i;
    }
    for(auto i = _savers.begin(); i != _savers.end(); i++) {
        delete *i;
    }
}

void FileManager::inputFile(std::string path)
{
    _in_file = path;
}

FILE* FileManager::logFile()
{
    return _log.fileHandler();
}

CalcServer::CalcServer* FileManager::load()
{
    unsigned int i, n=0;

    // Load the XML definition file
    if(_state.load(inputFile(), _simulation)){
        return NULL;
    }

    // Setup the problem setup
    if(_simulation.perform()) {
        return NULL;
    }

    // Build the calculation server
    CalcServer::CalcServer *C = new CalcServer::CalcServer(_simulation);
    if(!C)
        return NULL;
    if(C->setup()){
        delete C;
        return NULL;
    }

    // Now we can build the loaders/savers
    for(i = 0; i < _simulation.sets.size(); i++){
        if(!strcmp(_simulation.sets.at(i)->inputFormat(), "ASCII")){
            ASCII *loader = new ASCII(
                _simulation, n, _simulation.sets.at(i)->n(), i);
            if(!loader){
                delete C;
                return NULL;
            }
            _loaders.push_back((Particles*)loader);
        }
        else if(!strcmp(_simulation.sets.at(i)->inputFormat(), "FastASCII")){
            FastASCII *loader = new FastASCII(
                _simulation, n, _simulation.sets.at(i)->n(), i);
            if(!loader){
                delete C;
                return NULL;
            }
            _loaders.push_back((Particles*)loader);
        }
        else if(!strcmp(_simulation.sets.at(i)->inputFormat(), "VTK")){
            #ifdef HAVE_VTK
                VTK *loader = new VTK(
                    _simulation, n, _simulation.sets.at(i)->n(), i);
                if(!loader){
                    delete C;
                    return NULL;
                }
                _loaders.push_back((Particles*)loader);
            #else
                LOG(L_ERROR, "AQUAgpusph has been compiled without VTK format.\n");
                return NULL;
            #endif // HAVE_VTK
        }
        else{
            std::ostringstream msg;
            msg << "Unknow \"" << _simulation.sets.at(i)->inputFormat()
                << "\" input file format" << std::endl;
            LOG(L_ERROR, msg.str());
            delete C;
            return NULL;
        }
        if(!strcmp(_simulation.sets.at(i)->outputFormat(), "ASCII")){
            ASCII *saver = new ASCII(
                _simulation, n, _simulation.sets.at(i)->n(), i);
            if(!saver){
                delete C;
                return NULL;
            }
            _savers.push_back((Particles*)saver);
        }
        else if(!strcmp(_simulation.sets.at(i)->outputFormat(), "VTK")){
            #ifdef HAVE_VTK
                VTK *saver = new VTK(
                    _simulation, n, _simulation.sets.at(i)->n(), i);
                if(!saver){
                    delete C;
                    return NULL;
                }
                _savers.push_back((Particles*)saver);
            #else
                LOG(L_ERROR, "AQUAgpusph has been compiled without VTK format.\n");
                return NULL;
            #endif // HAVE_VTK
        }
        else{
            std::ostringstream msg;
            msg << "Unknow \"" << _simulation.sets.at(i)->outputFormat()
                << "\" input file format" << std::endl;
            LOG(L_ERROR, msg.str());
            delete C;
            return NULL;
        }
        n += _simulation.sets.at(i)->n();
    }

    // Execute the loaders
    for(auto loader = _loaders.begin(); loader != _loaders.end(); loader++) {
        if((*loader)->load()) {
            delete C;
            return NULL;
        }
    }

    return C;
}

bool FileManager::save()
{
    unsigned int i;

    // Execute the savers
    for(auto saver = _savers.begin(); saver != _savers.end(); saver++) {
        if((*saver)->save())
            return true;
    }

    // Save the XML definition file
    if(_state.save(_simulation, _savers)){
        return true;
    }

    return false;
}

}}  // namespace
