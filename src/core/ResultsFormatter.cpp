#include "ResultsFormatter.h"
#include "utilities/Normalizer.h"
#include "utilities/Utils.h"
#include "grid/Grid.h"
#include "grid/NormalCell.h"
#include "mesh/Mesh.h"

#include <boost/filesystem.hpp>
#include <iostream>

using namespace boost::filesystem;

ResultsFormatter::ResultsFormatter() {
    auto config = Config::getInstance();
    _root = config->getOutputFolder();
    _main = Utils::getCurrentDateAndTime() + "_" + config->getName();
    _scalarParams = {Param::PRESSURE, Param::DENSITY, Param::TEMPERATURE};
    _vectorParams = {Param::FLOW, Param::HEATFLOW};
}

void ResultsFormatter::writeAll(unsigned int iteration, Mesh* mesh, const std::vector<CellResults*>& results) {
    if (exists(_root) == false) {
        std::cout << "No such folder: " << _root << std::endl;
        return;
    }

    path mainPath{_root / _main};
    if (exists(mainPath) == false) {
        create_directory(mainPath);
    }

    path filePath = mainPath / (Utils::toString(iteration) + ".vtk"); // _types[Type::DATA] /
    std::ofstream fs(filePath.generic_string(), std::ios::out); //  | std::ios::binary

    // writing file
    fs << "# vtk DataFile Version 2.0" << std::endl; // header - version and identifier
    fs << "SAMPLE FILE" << std::endl; // title (256c max)
    fs << "ASCII" << std::endl; // data type (ASCII or BINARY)
    fs << "DATASET UNSTRUCTURED_GRID" << std::endl; // type of form
    fs << std::endl;

    // points
    fs << "POINTS " << mesh->getNodes().size() << " " << "double" << std::endl;
    double units = Config::getInstance()->getMeshUnits();
    for (const auto& node : mesh->getNodes()) {
        auto point = node->getPosition();
        double x = point.x() / units;
        double y = point.y() / units;
        double z = point.z() / units;
        fs << x << " " << y << " " << z << std::endl;
    }
    fs << std::endl;

    // cells
    std::vector<Element*> elements;
    for (const auto& element : mesh->getElements()) {
        auto pos = std::find_if(results.begin(), results.end(), [&element](CellResults* res) {
            return element->getId() == res->getId();
        });
        if (pos != results.end()) {
            elements.push_back(element.get());
        }
    }
    auto numberOfAllIndices = 0;
    for (auto element : elements) {
        numberOfAllIndices += element->getNodeIds().size();
        numberOfAllIndices += 1;
    }
    fs << "CELLS " << elements.size() << " " << numberOfAllIndices << std::endl;
    for (auto element : elements) {
        const auto& nodeIds = element->getNodeIds();
        fs << nodeIds.size();
        for (const auto& nodeId : nodeIds) {
            fs << " " << (nodeId - 1);
        }
        fs << std::endl;
    }
    fs << std::endl;

    // cell types
    fs << "CELL_TYPES " << elements.size() << std::endl;
    for (auto element : elements) {
        int cellType = 0;
        switch (element->getType()) {
            case Element::Type::POINT:
                cellType = 1;
                break;
            case Element::Type::LINE:
                cellType = 3;
                break;
            case Element::Type::TRIANGLE:
                cellType = 5;
                break;
            case Element::Type::QUADRANGLE:
                cellType = 9;
                break;
            case Element::Type::TETRAHEDRON:
                cellType = 10;
                break;
            case Element::Type::HEXAHEDRON:
                cellType = 12;
                break;
            case Element::Type::PRISM:
                cellType = 13;
                break;
        }
        fs << cellType << std::endl;
    }
    fs << std::endl;

    // scalar field (density, temperature, pressure) - lookup table "default"
    fs << "CELL_DATA " << elements.size() << std::endl;

    auto config = Config::getInstance();
    auto normalizer = config->getNormalizer();

    for (auto gi = 0; gi < config->getGases().size(); gi++) {
        for (auto param : _scalarParams) {
            std::string paramName;
            switch (param) {
                case Param::PRESSURE:
                    paramName = "Pressure";
                    break;
                case Param::DENSITY:
                    paramName = "Density";
                    break;
                case Param::TEMPERATURE:
                    paramName = "Temperature";
                    break;
            }
            paramName += "_" + Utils::toString(gi);

            fs << "SCALARS " << paramName << " " << "double" << " " << 1 << std::endl;
            fs << "LOOKUP_TABLE " << "default" << std::endl;
            for (auto element : elements) {
                double value = 0.0;
                auto pos = std::find_if(results.begin(), results.end(), [&element](CellResults* res) {
                    return element->getId() == res->getId();
                });
                if (pos != results.end()) {
                    switch (param) {
                        case Param::PRESSURE:
                            value = normalizer->restore((*pos)->getPressure(gi), Normalizer::Type::PRESSURE);
                            break;
                        case Param::DENSITY:
                            value = normalizer->restore((*pos)->getDensity(gi), Normalizer::Type::DENSITY);
                            break;
                        case Param::TEMPERATURE:
                            value = normalizer->restore((*pos)->getTemp(gi), Normalizer::Type::TEMPERATURE);
                            break;
                    }
                }
                fs << value << std::endl;
            }
        }

        for (auto param : _vectorParams) {
            std::string paramName;
            switch (param) {
                case Param::FLOW:
                    paramName = "Flow";
                    break;
                case Param::HEATFLOW:
                    paramName = "HeatFlow";
                    break;
            }
            paramName += "_" + Utils::toString(gi);

            fs << "VECTORS " << paramName << " " << "double" << std::endl;
            for (auto element : elements) {
                Vector3d value;
                auto pos = std::find_if(results.begin(), results.end(), [&element](CellResults* res) {
                    return element->getId() == res->getId();
                });
                if (pos != results.end()) {
                    switch (param) {
                        case Param::FLOW:
                            value = (*pos)->getFlow(gi);
                            normalizer->restore(value.x(), Normalizer::Type::FLOW);
                            normalizer->restore(value.y(), Normalizer::Type::FLOW);
                            normalizer->restore(value.z(), Normalizer::Type::FLOW);
                            break;
                        case Param::HEATFLOW:
                            value = (*pos)->getHeatFlow(gi);
                            normalizer->restore(value.x(), Normalizer::Type::HEATFLOW);
                            normalizer->restore(value.y(), Normalizer::Type::HEATFLOW);
                            normalizer->restore(value.z(), Normalizer::Type::HEATFLOW);
                            break;
                    }
                }
                fs << value.x() << " " << value.y() << " " << value.z() << std::endl;
            }
        }
    }

    fs.close();
}
