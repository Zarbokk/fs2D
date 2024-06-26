//
// Created by aya on 08.12.23.
//

#include "softRegistrationClass3D.h"

//bool compareTwoAngleCorrelation3D(rotationPeak3D i1, rotationPeak3D i2) {
//    return (i1.angle < i2.angle);
//}

double thetaIncrement3D(double index, int bandwidth) {
    return M_PI * index / (2.0 * (bandwidth - 1));
}

double phiIncrement3D(double index, int bandwidth) {
    return M_PI * index / bandwidth;
}

//double radiusIncrement(int index, int maximum, int minimum, int numberOfIncrements) {
//    return ((double) index / (double) numberOfIncrements) * ((double) (maximum - minimum)) + (double) minimum;
//}

double angleDifference3D(double angle1, double angle2) {//gives angle 1 - angle 2
    return atan2(sin(angle1 - angle2), cos(angle1 - angle2));
}

double
softRegistrationClass3D::getSpectrumFromVoxelData3D(double voxelData[], double magnitude[], double phase[],
                                                    bool gaussianBlur) {

    double *voxelDataTMP;
    voxelDataTMP = (double *) malloc(sizeof(double) * N * N * N);
    for (int i = 0; i < this->N * this->N * this->N; i++) {
        voxelDataTMP[i] = voxelData[i];
    }
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                int index = generalHelpfulTools::index3D(i, j, k, N);
                inputSpacialData[index][0] = voxelDataTMP[index]; // real part
                inputSpacialData[index][1] = 0; // imaginary part
            }
        }
    }

    fftw_execute(planVoxelToFourier3D);


    double maximumMagnitude = 0;


    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                int index = generalHelpfulTools::index3D(i, j, k, N);
                magnitude[index] = sqrt(
                        spectrumOut[index][0] * spectrumOut[index][0] + spectrumOut[index][1] * spectrumOut[index][1]);
                if (maximumMagnitude < magnitude[index]) {
                    maximumMagnitude = magnitude[index];
                }
                phase[index] = atan2(spectrumOut[index][1], spectrumOut[index][0]);
            }
        }
    }


    free(voxelDataTMP);
    return maximumMagnitude;
}


std::vector<rotationPeak3D>
softRegistrationClass3D::sofftRegistrationVoxel3DListOfPossibleRotations(double voxelData1Input[],
                                                                         double voxelData2Input[], bool debug,
                                                                         bool multipleRadii, bool useClahe,
                                                                         bool useHamming) {

    double maximumScan1Magnitude = this->getSpectrumFromVoxelData3D(voxelData1Input, this->magnitude1,
                                                                    this->phase1, false);
    double maximumScan2Magnitude = this->getSpectrumFromVoxelData3D(voxelData2Input, this->magnitude2,
                                                                    this->phase2, false);


    if (debug) {
        std::ofstream myFile1, myFile2, myFile3, myFile4, myFile5, myFile6;
        myFile1.open(
                "/home/tim-external/Documents/matlabTestEnvironment/registrationFourier/3D/csvFiles/magnitudeFFTW1.csv");
        myFile2.open(
                "/home/tim-external/Documents/matlabTestEnvironment/registrationFourier/3D/csvFiles/phaseFFTW1.csv");
        myFile3.open(
                "/home/tim-external/Documents/matlabTestEnvironment/registrationFourier/3D/csvFiles/voxelDataFFTW1.csv");
        myFile4.open(
                "/home/tim-external/Documents/matlabTestEnvironment/registrationFourier/3D/csvFiles/magnitudeFFTW2.csv");
        myFile5.open(
                "/home/tim-external/Documents/matlabTestEnvironment/registrationFourier/3D/csvFiles/phaseFFTW2.csv");
        myFile6.open(
                "/home/tim-external/Documents/matlabTestEnvironment/registrationFourier/3D/csvFiles/voxelDataFFTW2.csv");
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                for (int k = 0; k < N; k++) {
                    int index = generalHelpfulTools::index3D(i, j, k, N);
                    myFile1 << this->magnitude1[index];
                    myFile1 << "\n";
                    myFile2 << this->phase1[index];
                    myFile2 << "\n";
                    myFile3 << voxelData1Input[index];
                    myFile3 << "\n";
                    myFile4 << this->magnitude2[index];
                    myFile4 << "\n";
                    myFile5 << this->phase2[index];
                    myFile5 << "\n";
                    myFile6 << voxelData2Input[index];
                    myFile6 << "\n";
                }
            }
        }

        myFile1.close();
        myFile2.close();
        myFile3.close();
        myFile4.close();
        myFile5.close();
        myFile6.close();
    }


    double globalMaximumMagnitude;
    if (maximumScan2Magnitude < maximumScan1Magnitude) {
        globalMaximumMagnitude = maximumScan1Magnitude;
    } else {
        globalMaximumMagnitude = maximumScan2Magnitude;
    }
    double minMagnitude = INFINITY;
    //normalize and shift both spectrums
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {

                int index = generalHelpfulTools::index3D(i, j, k, N);

                int indexX = (N / 2 + i) % N;
                int indexY = (N / 2 + j) % N;
                int indexZ = (N / 2 + k) % N;

                int indexshift = generalHelpfulTools::index3D(indexX, indexY, indexZ, N);
                if (minMagnitude > magnitude1[index]) {
                    minMagnitude = magnitude1[index];
                }
                if (minMagnitude > magnitude2[index]) {
                    minMagnitude = magnitude2[index];
                }
                magnitude1Shifted[indexshift] =
                        magnitude1[index] / globalMaximumMagnitude;
                magnitude2Shifted[indexshift] =
                        magnitude2[index] / globalMaximumMagnitude;
            }
        }
    }


    for (int i = 0; i < N * N; i++) {
        resampledMagnitudeSO3_1[i] = 0;
        resampledMagnitudeSO3_2[i] = 0;
        resampledMagnitudeSO3_1TMP[i] = 0;
        resampledMagnitudeSO3_2TMP[i] = 0;
    }

    int bandwidth = N / 2;
    double minValue = INFINITY;
    double maxValue = 0;

    for (int r = N/16; r < N / 2 - N / 8; r++) {

        for (int i = 0; i < N * N; i++) {
            resampledMagnitudeSO3_1TMP[i] = 0;
            resampledMagnitudeSO3_2TMP[i] = 0;
        }
        for (int i = 0; i < 2 * bandwidth; i++) {
            for (int j = 0; j < 2 * bandwidth; j++) {



                double theta = thetaIncrement3D((double) i, bandwidth);
                double phi = phiIncrement3D((double) j, bandwidth);
                double radius = r;

                int xIndex = std::round(radius * std::sin(theta) * std::cos(phi) + N / 2 + 0.1);
                int yIndex = std::round(radius * std::sin(theta) * std::sin(phi) + N / 2 + 0.1);
                int zIndex = std::round(radius * std::cos(theta) + N / 2 + 0.1);



                resampledMagnitudeSO3_1TMP[generalHelpfulTools::index2D(i, j, bandwidth * 2)] =
                        magnitude1Shifted[generalHelpfulTools::index3D(xIndex, yIndex, zIndex, bandwidth * 2)];
                resampledMagnitudeSO3_2TMP[generalHelpfulTools::index2D(i, j, bandwidth * 2)] =
                        magnitude2Shifted[generalHelpfulTools::index3D(xIndex, yIndex, zIndex, bandwidth * 2)] ;

            }
        }

        for (int i = 0; i < 2 * bandwidth; i++) {
            for (int j = 0; j < 2 * bandwidth; j++) {

                if (minValue > resampledMagnitudeSO3_1TMP[generalHelpfulTools::index2D(i, j, bandwidth * 2)]) {
                    minValue = resampledMagnitudeSO3_1TMP[generalHelpfulTools::index2D(i, j, bandwidth * 2)];
                }
                if (maxValue < resampledMagnitudeSO3_1TMP[generalHelpfulTools::index2D(i, j, bandwidth * 2)]) {
                    maxValue = resampledMagnitudeSO3_1TMP[generalHelpfulTools::index2D(i, j, bandwidth * 2)];
                }
                if (minValue > resampledMagnitudeSO3_2TMP[generalHelpfulTools::index2D(i, j, bandwidth * 2)]) {
                    minValue = resampledMagnitudeSO3_2TMP[generalHelpfulTools::index2D(i, j, bandwidth * 2)];
                }
                if (maxValue < resampledMagnitudeSO3_2TMP[generalHelpfulTools::index2D(i, j, bandwidth * 2)]) {
                    maxValue = resampledMagnitudeSO3_2TMP[generalHelpfulTools::index2D(i, j, bandwidth * 2)];
                }
            }
        }
        for (int i = 0; i < 2 * bandwidth; i++) {
            for (int j = 0; j < 2 * bandwidth; j++) {
                this->resampledMagnitudeSO3_1TMP[generalHelpfulTools::index2D(i, j, 2 * bandwidth)] =
                        255.0*((this->resampledMagnitudeSO3_1TMP[generalHelpfulTools::index2D(i, j, 2 * bandwidth)] - minValue) /
                        (maxValue - minValue));
                this->resampledMagnitudeSO3_2TMP[generalHelpfulTools::index2D(i, j, 2 * bandwidth)] =
                        255.0*((this->resampledMagnitudeSO3_2TMP[generalHelpfulTools::index2D(i, j, 2 * bandwidth)] - minValue) /
                        (maxValue - minValue));
            }
        }

        cv::Mat magTMP1(N, N, CV_64FC1, resampledMagnitudeSO3_1TMP);
        cv::Mat magTMP2(N, N, CV_64FC1, resampledMagnitudeSO3_2TMP);
        magTMP1.convertTo(magTMP1, CV_8UC1);
        magTMP2.convertTo(magTMP2, CV_8UC1);

//        cv::imshow("b1", magTMP1);
//        cv::imshow("b2", magTMP2);
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
        clahe->setClipLimit(2);
        if (useClahe) {
            clahe->apply(magTMP1, magTMP1);
            clahe->apply(magTMP2, magTMP2);
        }
//        cv::imshow("a1", magTMP1);
//        cv::imshow("a2", magTMP2);
//        int k = cv::waitKey(0); // Wait for a keystroke in the window

        for (int i = 0; i < 2 * bandwidth; i++) {
            for (int j = 0; j < 2 * bandwidth; j++) {
//                    hammingCoeff = 25.0 / 46.0 - (1.0 - 25.0 / 46.0) * cos(2 * M_PI * j / (2 * bandwidth));
                resampledMagnitudeSO3_1[generalHelpfulTools::index2D(i, j, bandwidth * 2)] +=
                        ((double) magTMP1.data[generalHelpfulTools::index2D(i, j, bandwidth * 2)]) /
                        255.0 ;
                resampledMagnitudeSO3_2[generalHelpfulTools::index2D(i, j, bandwidth * 2)] +=
                        ((double) magTMP2.data[generalHelpfulTools::index2D(i, j, bandwidth * 2)]) /
                        255.0 ;
            }
        }
    }
    if (debug) {
        std::ofstream myFile1, myFile2;
        myFile1.open(
                "/home/tim-external/Documents/matlabTestEnvironment/registrationFourier/3D/csvFiles/resampledMagnitudeSO3_1.csv");
        myFile2.open(
                "/home/tim-external/Documents/matlabTestEnvironment/registrationFourier/3D/csvFiles/resampledMagnitudeSO3_2.csv");
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                myFile1 << this->resampledMagnitudeSO3_1[generalHelpfulTools::index2D(i, j,
                                                                                      bandwidth * 2)]; // real part
                myFile1 << "\n";
                myFile2 << this->resampledMagnitudeSO3_2[generalHelpfulTools::index2D(i, j,
                                                                                      bandwidth * 2)]; // imaginary part
                myFile2 << "\n";
            }
        }
        myFile1.close();
        myFile2.close();
    }


    this->sofftCorrelationObject3D.correlationOfTwoSignalsInSO3(resampledMagnitudeSO3_1, resampledMagnitudeSO3_2,
                                                                resultingCorrelationComplex);

    if (debug) {
        std::ofstream myFile1, myFile2;
        myFile1.open(
                "/home/tim-external/Documents/matlabTestEnvironment/registrationFourier/3D/csvFiles/resultingCorrelationReal.csv");
        myFile2.open(
                "/home/tim-external/Documents/matlabTestEnvironment/registrationFourier/3D/csvFiles/resultingCorrelationComplex.csv");
        for (int i = 0; i < bwOut * 2; i++) {
            for (int j = 0; j < bwOut * 2; j++) {
                for (int k = 0; k < bwOut * 2; k++) {
                    int index = generalHelpfulTools::index3D(i, j, k, bwOut * 2);
                    myFile1 << this->resultingCorrelationComplex[index][0]; // real part
                    myFile1 << "\n";
                    myFile2 << this->resultingCorrelationComplex[index][1]; // imaginary part
                    myFile2 << "\n";
                }
            }
        }
        myFile1.close();
        myFile2.close();
    }

    double minimumCorrelation = INFINITY;
    double maximumCorrelation = 0;
    for (int i = 0; i < 8 * bwOut * bwOut * bwOut; i++) {
        if (minimumCorrelation > NORM(resultingCorrelationComplex[i])) {
            minimumCorrelation = NORM(resultingCorrelationComplex[i]);
        }
        if (maximumCorrelation < NORM(resultingCorrelationComplex[i])) {
            maximumCorrelation = NORM(resultingCorrelationComplex[i]);
        }
    }


    double correlationCurrent;
    std::vector<My4DPoint> listOfQuaternionCorrelation;
    for (int i = 0; i < bwOut * 2; i++) {
        for (int j = 0; j < bwOut * 2; j++) {
            for (int k = 0; k < bwOut * 2; k++) {

                correlationCurrent =
                        (NORM(resultingCorrelationComplex[generalHelpfulTools::index3D(i, j, k, 2 * bwOut)]) -
                         minimumCorrelation) /
                        (maximumCorrelation - minimumCorrelation);
                Eigen::AngleAxisd rotation_vectorz1(k * 2 * M_PI / (N), Eigen::Vector3d(0, 0, 1));
                Eigen::AngleAxisd rotation_vectory(i * M_PI / (N), Eigen::Vector3d(0, 1, 0));
                Eigen::AngleAxisd rotation_vectorz2(j * 2 * M_PI / (N), Eigen::Vector3d(0, 0, 1));

                Eigen::Matrix3d tmpMatrix3d =
                        rotation_vectorz1.toRotationMatrix().inverse() *
                        rotation_vectory.toRotationMatrix().inverse() *
                        rotation_vectorz2.toRotationMatrix().inverse();
                Eigen::Quaterniond quaternionResult(tmpMatrix3d);
                quaternionResult.normalize();

                if (quaternionResult.w() < 0) {
                    Eigen::Quaterniond tmpQuad = quaternionResult;
                    quaternionResult.w() = -tmpQuad.w();
                    quaternionResult.x() = -tmpQuad.x();
                    quaternionResult.y() = -tmpQuad.y();
                    quaternionResult.z() = -tmpQuad.z();
                }
                My4DPoint currentPoint;
                currentPoint.correlation = correlationCurrent;
                currentPoint[0] = quaternionResult.w();
                currentPoint[1] = quaternionResult.x();
                currentPoint[2] = quaternionResult.y();
                currentPoint[3] = quaternionResult.z();

                listOfQuaternionCorrelation.push_back(currentPoint);
            }
        }
    }

    std::vector<rotationPeak4D> potentialRotationsTMP = this->peakDetectionOf4DCorrelationWithKDTreeFindPeaksLibrary(
            listOfQuaternionCorrelation);

    for (int i = 0; i < potentialRotationsTMP.size(); i++) {

        Eigen::Quaterniond rotationQuat(potentialRotationsTMP[i].w, potentialRotationsTMP[i].x,
                                        potentialRotationsTMP[i].y, potentialRotationsTMP[i].z);
        Eigen::Vector3d rpyCurrentRot = generalHelpfulTools::getRollPitchYaw(rotationQuat);

        std::cout << i << " , " << potentialRotationsTMP[i].levelPotential << " , "
                  << potentialRotationsTMP[i].correlationHeight
                  << " , " << potentialRotationsTMP[i].persistence << " , " << rpyCurrentRot[0] * 180 / M_PI << " , "
                  << rpyCurrentRot[1] * 180 / M_PI << " , " << rpyCurrentRot[2] * 180 / M_PI << " , "
                  << potentialRotationsTMP[i].x << " , " << potentialRotationsTMP[i].y << " , "
                  << potentialRotationsTMP[i].z << " , " << potentialRotationsTMP[i].w << std::endl;
    }


//    for (int p = 0; p < potentialRotationsTMP.size(); p++) {
    for (int p = 0; p < 1; p++) {

        double *voxelData2Rotated;
        voxelData2Rotated = (double *) malloc(sizeof(double) * this->N * this->N * this->N);

        Eigen::Quaterniond currentRotation(potentialRotationsTMP[p].w, potentialRotationsTMP[p].x,
                                           potentialRotationsTMP[p].y, potentialRotationsTMP[p].z);
//        std::cout << currentRotation << std::endl;

        for (int i = 0; i < this->N; i++) {
            for (int j = 0; j < this->N; j++) {
                for (int k = 0; k < this->N; k++) {
                    int index = generalHelpfulTools::index3D(i, j, k, this->N);
                    int xCoordinate = i - this->N / 2;
                    int yCoordinate = j - this->N / 2;
                    int zCoordinate = k - this->N / 2;
                    Eigen::Vector3d newCoordinate(xCoordinate, yCoordinate, zCoordinate);
                    newCoordinate = currentRotation * newCoordinate;
                    Eigen::Vector3d lookUpVector = newCoordinate;
                    double occupancyValue = getPixelValueInterpolated(lookUpVector, voxelData2Input);
                    voxelData2Rotated[index] = occupancyValue;
                }
            }
        }

        double maximumScan1CorrelationMagnitude = this->getSpectrumFromVoxelData3D(voxelData1Input,
                                                                                   this->magnitude1Correlation,
                                                                                   this->phase1Correlation, false);
        double maximumScan2CorrelationMagnitude = this->getSpectrumFromVoxelData3D(voxelData2Rotated,
                                                                                   this->magnitude2Correlation,
                                                                                   this->phase2Correlation, false);


        if (debug) {
            std::ofstream myFile1, myFile2, myFile3;
            myFile1.open(
                    "/home/tim-external/Documents/matlabTestEnvironment/registrationFourier/3D/csvFiles/magnitudeFFTW2Rotated.csv");
            myFile2.open(
                    "/home/tim-external/Documents/matlabTestEnvironment/registrationFourier/3D/csvFiles/phaseFFTW2Rotated.csv");
            myFile3.open(
                    "/home/tim-external/Documents/matlabTestEnvironment/registrationFourier/3D/csvFiles/voxelDataFFTW2Rotated.csv");
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < N; j++) {
                    for (int k = 0; k < N; k++) {
                        int index = generalHelpfulTools::index3D(i, j, k, N);
                        myFile1 << this->magnitude2Correlation[index];
                        myFile1 << "\n";
                        myFile2 << this->phase2Correlation[index];
                        myFile2 << "\n";
                        myFile3 << voxelData2Rotated[index];
                        myFile3 << "\n";
                    }
                }
            }

            myFile1.close();
            myFile2.close();
            myFile3.close();
        }

        //calculate correlation of spectrums
        for (int i = 0; i < this->correlationN; i++) {
            for (int j = 0; j < this->correlationN; j++) {
                for (int k = 0; k < this->correlationN; k++) {
                    int indexX = i;
                    int indexY = j;
                    int indexZ = k;
                    int index = generalHelpfulTools::index3D(indexX, indexY, indexZ, this->correlationN);
                    //calculate the spectrum back
                    std::complex<double> tmpComplex1 =
                            magnitude1Correlation[index] *
                            std::exp(std::complex<double>(0, phase1Correlation[index]));
                    std::complex<double> tmpComplex2 =
                            magnitude2Correlation[index] *
                            std::exp(std::complex<double>(0, phase2Correlation[index]));
                    std::complex<double> resultComplex = ((tmpComplex1) * conj(tmpComplex2));
                    resultingPhaseDiff3DCorrelation[index][0] = resultComplex.real();
                    resultingPhaseDiff3DCorrelation[index][1] = resultComplex.imag();
                }
            }
        }
        // back fft
        fftw_execute(planFourierToVoxel3DCorrelation);
        // fftshift and calc magnitude
        double maximumCorrelation = 0;
        for (int i = 0; i < this->correlationN; i++) {
            for (int j = 0; j < this->correlationN; j++) {
                for (int k = 0; k < this->correlationN; k++) {
                    int indexX = (this->correlationN / 2 + i + this->correlationN) %
                                 this->correlationN;// changed j and i here
                    int indexY = (this->correlationN / 2 + j + this->correlationN) % this->correlationN;
                    int indexZ = (this->correlationN / 2 + k + this->correlationN) % this->correlationN;
                    int indexShifted = generalHelpfulTools::index3D(indexX, indexY, indexZ, this->correlationN);
                    int index = generalHelpfulTools::index3D(i, j, k, this->correlationN);
                    //maybe without sqrt, but for now thats fine
//                    double normalizationFactorForCorrelation =
//                            1 / this->normalizationFactorCalculation(indexX, indexY, indexZ);
                    double normalizationFactorForCorrelation = 1;
//            double normalizationFactorForCorrelation = 1/this->normalizationFactorCalculation(indexX, indexY);
                    normalizationFactorForCorrelation = sqrt(normalizationFactorForCorrelation);

                    resultingCorrelationDouble[indexShifted] =
                            normalizationFactorForCorrelation * NORM(resultingShiftPeaks3DCorrelation[index]); // magnitude;

                    if (maximumCorrelation < resultingCorrelationDouble[indexShifted]) {
                        maximumCorrelation = resultingCorrelationDouble[indexShifted];
                    }
                }
            }
        }


        if (debug) {
            std::ofstream myFile10;
            myFile10.open(
                    "/home/tim-external/Documents/matlabTestEnvironment/registrationFourier/3D/csvFiles/resultingCorrelationShift.csv");
            for (int i = 0; i < this->correlationN; i++) {
                for (int j = 0; j < this->correlationN; j++) {
                    for (int k = 0; k < this->correlationN; k++) {

                        myFile10 << resultingCorrelationDouble[generalHelpfulTools::index3D(i, j, k,
                                                                                            this->correlationN)];
                        myFile10 << "\n";
                    }
                }
            }
            myFile10.close();
        }

        std::vector<rotationPeak3D> resulting3DPeakList = peakDetectionOf3DCorrelationFindPeaksLibrary(
                resultingCorrelationDouble, this->correlationN, 1);

        for (int i = 0; i < resulting3DPeakList.size(); i++) {
            std::cout << i << " , " << resulting3DPeakList[i].levelPotential << " , "
                      << resulting3DPeakList[i].correlationHeight << " , " << resulting3DPeakList[i].persistence
                      << " , " << resulting3DPeakList[i].xTranslation << " , " << resulting3DPeakList[i].yTranslation
                      << " , " << resulting3DPeakList[i].zTranslation << std::endl;
        }


        free(voxelData2Rotated);
    }


    exit(161);
    std::vector<rotationPeak3D> potentialRotations = this->peakDetectionOf3DCorrelationFindPeaksLibraryFromFFTW_COMPLEX(
            resultingCorrelationComplex,
            1);


    return potentialRotations;

}

//int getIndexOfData(int x, int y, int z, int N_input) {
//    if (x < 0 || x > N_input) {
//        return -1;
//    }
//    if (y < 0 || y > N_input) {
//        return -1;
//    }
//    if (z < 0 || z > N_input) {
//        return -1;
//    }
//    return z + N_input * y + N_input * N_input * x;
//}

double softRegistrationClass3D::normalizationFactorCalculation(int x, int y, int z) {

    double tmpCalculation = 0;
//    tmpCalculation = abs(1.0/((x-this->correlationN/2)*(y-this->correlationN/2)));
//    tmpCalculation = this->correlationN * this->correlationN * (this->correlationN - (x + 1) + 1);
//    tmpCalculation = tmpCalculation * (this->correlationN - (y + 1) + 1);
    if (x < ceil(this->correlationN / 2)) {
        tmpCalculation = (x + 1);
    } else {
        tmpCalculation = (this->correlationN - x);
    }

    if (y < ceil(this->correlationN / 2)) {
        tmpCalculation = tmpCalculation * (y + 1);
    } else {
        tmpCalculation = tmpCalculation * (this->correlationN - y);
    }

    if (z < ceil(this->correlationN / 2)) {
        tmpCalculation = tmpCalculation * (z + 1);
    } else {
        tmpCalculation = tmpCalculation * (this->correlationN - z);
    }

    return (tmpCalculation);
}

double softRegistrationClass3D::getPixelValueInterpolated(Eigen::Vector3d positionVector, double *volumeData) {

//    int index = positionVector.x() + N * positionVector.y() + N * N * positionVector.z();
//    std::cout << "test1" << std::endl;
    int xDown = floor(positionVector.x());
    int xUp = ceil(positionVector.x());

    int yDown = floor(positionVector.y());
    int yUp = ceil(positionVector.y());

    int zDown = floor(positionVector.z());
    int zUp = ceil(positionVector.z());

    int xDownCorrected = xDown + this->N / 2;
    int xUpCorrected = xUp + this->N / 2;

    int yDownCorrected = yDown + this->N / 2;
    int yUpCorrected = yUp + this->N / 2;

    int zDownCorrected = zDown + this->N / 2;
    int zUpCorrected = zUp + this->N / 2;


    double dist1 = (positionVector - Eigen::Vector3d(xDown, yDown, zDown)).norm();
    double dist2 = (positionVector - Eigen::Vector3d(xDown, yDown, zUp)).norm();
    double dist3 = (positionVector - Eigen::Vector3d(xDown, yUp, zDown)).norm();
    double dist4 = (positionVector - Eigen::Vector3d(xDown, yUp, zUp)).norm();
    double dist5 = (positionVector - Eigen::Vector3d(xUp, yDown, zDown)).norm();
    double dist6 = (positionVector - Eigen::Vector3d(xUp, yDown, zUp)).norm();
    double dist7 = (positionVector - Eigen::Vector3d(xUp, yUp, zDown)).norm();
    double dist8 = (positionVector - Eigen::Vector3d(xUp, yUp, zUp)).norm();




//    double fullLength = dist1 + dist2 + dist3 + dist4 + dist5 + dist6 + dist7 + dist8;


    int index1 = generalHelpfulTools::index3D(xDownCorrected, yDownCorrected, zDownCorrected, this->N);
    int index2 = generalHelpfulTools::index3D(xDownCorrected, yDownCorrected, zUpCorrected, this->N);
    int index3 = generalHelpfulTools::index3D(xDownCorrected, yUpCorrected, zDownCorrected, this->N);
    int index4 = generalHelpfulTools::index3D(xDownCorrected, yUpCorrected, zUpCorrected, this->N);
    int index5 = generalHelpfulTools::index3D(xUpCorrected, yDownCorrected, zDownCorrected, this->N);
    int index6 = generalHelpfulTools::index3D(xUpCorrected, yDownCorrected, zUpCorrected, this->N);
    int index7 = generalHelpfulTools::index3D(xUpCorrected, yUpCorrected, zDownCorrected, this->N);
    int index8 = generalHelpfulTools::index3D(xUpCorrected, yUpCorrected, zUpCorrected, this->N);


    double correlationValue1;
    double correlationValue2;
    double correlationValue3;
    double correlationValue4;
    double correlationValue5;
    double correlationValue6;
    double correlationValue7;
    double correlationValue8;
//    std::cout << "test2" << std::endl;
    if (index1 == -1) {
        correlationValue1 = 0;
    } else {
        correlationValue1 = volumeData[index1];
    }
    if (index2 == -1) {
        correlationValue2 = 0;
    } else {
        correlationValue2 = volumeData[index2];
    }
    if (index3 == -1) {
        correlationValue3 = 0;
    } else {
        correlationValue3 = volumeData[index3];
    }
    if (index4 == -1) {
        correlationValue4 = 0;
    } else {
        correlationValue4 = volumeData[index4];
    }
    if (index5 == -1) {
        correlationValue5 = 0;
    } else {
        correlationValue5 = volumeData[index5];
    }
    if (index6 == -1) {
        correlationValue6 = 0;
    } else {
        correlationValue6 = volumeData[index6];
    }
    if (index7 == -1) {
        correlationValue7 = 0;
    } else {
        correlationValue7 = volumeData[index7];
    }
    if (index8 == -1) {
        correlationValue8 = 0;
    } else {
        correlationValue8 = volumeData[index8];
    }

    double e = 1.0 / 100000000.0;
    if (dist1 < e) {
        return correlationValue1;
    }
    if (dist2 < e) {
        return correlationValue2;
    }
    if (dist3 < e) {
        return correlationValue3;
    }
    if (dist4 < e) {
        return correlationValue4;
    }
    if (dist5 < e) {
        return correlationValue5;
    }
    if (dist6 < e) {
        return correlationValue6;
    }
    if (dist7 < e) {
        return correlationValue7;
    }
    if (dist8 < e) {
        return correlationValue8;
    }

//    std::cout << "test3" << std::endl;
    // inverse weighted sum
    double correlationValue = (correlationValue1 / dist1 + correlationValue2 / dist2 + correlationValue3 / dist3 +
                               correlationValue4 / dist4 + correlationValue5 / dist5 + correlationValue6 / dist6 +
                               correlationValue7 / dist7 + correlationValue8 / dist8) /
                              (1 / dist1 + 1 / dist2 + 1 / dist3 + 1 / dist4 + 1 / dist5 + 1 / dist6 + 1 / dist7 +
                               1 / dist8);

//    if(correlationValue>0.1 && correlationValue < 0.9){
//        std::cout << "test" << std::endl;
//    }
//    std::cout << "test4" << std::endl;
    return correlationValue;


}


std::vector<rotationPeak3D>
softRegistrationClass3D::peakDetectionOf3DCorrelationFindPeaksLibraryFromFFTW_COMPLEX(fftw_complex *inputcorrelation,
                                                                                      double cellSize) {

    double *current3DCorrelation;
    current3DCorrelation = (double *) malloc(
            sizeof(double) * this->correlationN * this->correlationN * this->correlationN);

    double maxValue = 0;
    double minValue = INFINITY;
    //copy data
    for (int j = 0; j < this->correlationN; j++) {
        for (int i = 0; i < this->correlationN; i++) {
            for (int k = 0; k < this->correlationN; k++) {


                double currentCorrelation =
                        (inputcorrelation[generalHelpfulTools::index3D(i, j, k, this->correlationN)][0]) *
                        (inputcorrelation[generalHelpfulTools::index3D(i, j, k, this->correlationN)][0]) +
                        (inputcorrelation[generalHelpfulTools::index3D(i, j, k, this->correlationN)][1]) *
                        (inputcorrelation[generalHelpfulTools::index3D(i, j, k, this->correlationN)][1]);
                current3DCorrelation[generalHelpfulTools::index3D(i, j, k, this->correlationN)] = currentCorrelation;
                if (current3DCorrelation[generalHelpfulTools::index3D(i, j, k, this->correlationN)] >
                    maxValue) {
                    maxValue = current3DCorrelation[generalHelpfulTools::index3D(i, j, k, this->correlationN)];
                }
                if (current3DCorrelation[generalHelpfulTools::index3D(i, j, k, this->correlationN)] <
                    minValue) {
                    minValue = current3DCorrelation[generalHelpfulTools::index3D(i, j, k, this->correlationN)];
                }
            }
        }
    }
    //normalize data
    for (int j = 0; j < this->correlationN; j++) {
        for (int i = 0; i < this->correlationN; i++) {
            for (int k = 0; k < this->correlationN; k++) {
                current3DCorrelation[generalHelpfulTools::index3D(i, j, k, this->correlationN)] =
                        (current3DCorrelation[generalHelpfulTools::index3D(i, j, k, this->correlationN)] - minValue) /
                        (maxValue - minValue);
            }
        }
    }
    cv::Mat magTMP1(this->correlationN, this->correlationN, CV_64F, current3DCorrelation);
//    cv::GaussianBlur(magTMP1, magTMP1, cv::Size(9, 9), 0);

//    cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE,
//                                            cv::Size(ceil(0.02 * this->correlationN), ceil(0.02 * this->correlationN)));
//    cv::morphologyEx(magTMP1, magTMP1, cv::MORPH_TOPHAT, element);

    size_t ourSize = this->correlationN;
    findpeaks::volume_t<double> volume = {
            ourSize, ourSize, ourSize,
            current3DCorrelation
    };

    std::vector<findpeaks::peak_3d<double>> peaks = findpeaks::persistance3d(volume);
    std::vector<rotationPeak3D> tmpTranslations;
    std::cout << peaks.size() << std::endl;
    int numberOfPeaks = 0;
    for (const auto &p: peaks) {
        //calculation of level, that is a potential translation
        double levelPotential = p.persistence * sqrt(p.birth_level) *
                                Eigen::Vector3d((double) ((int) p.birth_position.x - (int) p.death_position.x),
                                                (double) ((int) p.birth_position.y - (int) p.death_position.y),
                                                (double) ((int) p.birth_position.z - (int) p.death_position.z)).norm() /
                                this->correlationN / 1.73205080757;

        std::cout << p.persistence << std::endl;
        std::cout << Eigen::Vector3d((double) ((int) p.birth_position.x - (int) p.death_position.x),
                                     (double) ((int) p.birth_position.y - (int) p.death_position.y),
                                     (double) ((int) p.birth_position.z - (int) p.death_position.z)).norm()
                  << std::endl;
        std::cout << sqrt(p.birth_level) << std::endl;


        rotationPeak3D tmpTranslationPeak;
        tmpTranslationPeak.xTranslation = p.birth_position.x;
        tmpTranslationPeak.yTranslation = p.birth_position.y;
        tmpTranslationPeak.zTranslation = p.birth_position.z;
        tmpTranslationPeak.persistence = p.persistence;
        tmpTranslationPeak.correlationHeight = current3DCorrelation[generalHelpfulTools::index3D(p.birth_position.x,
                                                                                                 p.birth_position.y,
                                                                                                 p.birth_position.z,
                                                                                                 this->correlationN)];
        tmpTranslationPeak.levelPotential = levelPotential;
        if (levelPotential > 0.1) {
            tmpTranslations.push_back(tmpTranslationPeak);
            std::cout << "test" << std::endl;
            numberOfPeaks++;
        }


    }
    free(current3DCorrelation);
    return (tmpTranslations);

}

std::vector<rotationPeak3D>
softRegistrationClass3D::peakDetectionOf3DCorrelationFindPeaksLibrary(double *inputcorrelation, int dimensionN,
                                                                      double cellSize) {

    double *current3DCorrelation;
    current3DCorrelation = (double *) malloc(
            sizeof(double) * dimensionN * dimensionN * dimensionN);

    double maxValue = 0;
    double minValue = INFINITY;
    //copy data
    for (int i = 0; i < dimensionN; i++) {
        for (int j = 0; j < dimensionN; j++) {
            for (int k = 0; k < dimensionN; k++) {
                int index = generalHelpfulTools::index3D(i, j, k, dimensionN);
                double currentCorrelation = inputcorrelation[index];
                current3DCorrelation[index] = currentCorrelation;
                if (currentCorrelation > maxValue) {
                    maxValue = currentCorrelation;
                }
                if (currentCorrelation < minValue) {
                    minValue = currentCorrelation;
                }
            }
        }
    }
    //normalize data
    for (int i = 0; i < dimensionN; i++) {
        for (int j = 0; j < dimensionN; j++) {
            for (int k = 0; k < dimensionN; k++) {
                int index = generalHelpfulTools::index3D(i, j, k, dimensionN);
                current3DCorrelation[index] = (current3DCorrelation[index] - minValue) / (maxValue - minValue);
            }
        }
    }

    size_t ourSize = dimensionN;
    findpeaks::volume_t<double> volume = {
            ourSize, ourSize, ourSize,
            current3DCorrelation
    };

    std::vector<findpeaks::peak_3d<double>> peaks = findpeaks::persistance3d(volume);
    std::vector<rotationPeak3D> tmpTranslations;
    int numberOfPeaks = 0;
    for (const auto &p: peaks) {
        //calculation of level, that is a potential translation
        double currentPersistence;
        if (p.persistence == INFINITY) {
            currentPersistence = 1;
        } else {
            currentPersistence = p.persistence;
        }



        double levelPotential = currentPersistence * sqrt(p.birth_level) *
                                Eigen::Vector3d((double) ((int) p.birth_position.x - (int) p.death_position.x),
                                                (double) ((int) p.birth_position.y - (int) p.death_position.y),
                                                (double) ((int) p.birth_position.z - (int) p.death_position.z)).norm() /
                                this->correlationN / 1.73205080757;

        rotationPeak3D tmpTranslationPeak;
        tmpTranslationPeak.xTranslation = -(double)((double)p.birth_position.x-(double)dimensionN/2.0);
        tmpTranslationPeak.yTranslation = -(double)((double)p.birth_position.y-(double)dimensionN/2.0);
        tmpTranslationPeak.zTranslation = -(double)((double)p.birth_position.z-(double)dimensionN/2.0);
        tmpTranslationPeak.persistence = currentPersistence;
        tmpTranslationPeak.correlationHeight = current3DCorrelation[generalHelpfulTools::index3D(p.birth_position.x,
                                                                                                 p.birth_position.y,
                                                                                                 p.birth_position.z,
                                                                                                 dimensionN)];
        tmpTranslationPeak.levelPotential = levelPotential;
        if (levelPotential > 0.001) {
            tmpTranslations.push_back(tmpTranslationPeak);
            numberOfPeaks++;
        }
    }
    free(current3DCorrelation);
    return (tmpTranslations);
}

std::vector<rotationPeak4D>
softRegistrationClass3D::peakDetectionOf4DCorrelationFindPeaksLibrary(double *inputcorrelation, long dimensionN,
                                                                      double cellSize) {

    double *current4DCorrelation;
    current4DCorrelation = (double *) malloc(
            sizeof(double) * dimensionN * dimensionN * dimensionN * dimensionN);

    double maxValue = 0;
    double minValue = INFINITY;
    //copy data
    for (int j = 0; j < dimensionN; j++) {
        for (int i = 0; i < dimensionN; i++) {
            for (int k = 0; k < dimensionN; k++) {
                for (int l = 0; l < dimensionN; l++) {
                    double currentCorrelation = inputcorrelation[j + dimensionN * i +
                                                                 k * dimensionN * dimensionN +
                                                                 l * dimensionN * dimensionN * dimensionN];
                    current4DCorrelation[j + dimensionN * i + k * dimensionN * dimensionN +
                                         l * dimensionN * dimensionN * dimensionN] = currentCorrelation;
                    if (currentCorrelation > maxValue) {
                        maxValue = currentCorrelation;
                    }
                    if (currentCorrelation < minValue) {
                        minValue = currentCorrelation;
                    }
                }
            }
        }
    }
    //normalize data
    for (int j = 0; j < dimensionN; j++) {
        for (int i = 0; i < dimensionN; i++) {
            for (int k = 0; k < dimensionN; k++) {
                for (int l = 0; l < dimensionN; l++) {
                    current4DCorrelation[j + dimensionN * i + k * dimensionN * dimensionN +
                                         l * dimensionN * dimensionN * dimensionN] =
                            (current4DCorrelation[j + dimensionN * i + k * dimensionN * dimensionN +
                                                  l * dimensionN * dimensionN * dimensionN] - minValue) /
                            (maxValue - minValue);
                }
            }
        }
    }
//    cv::Mat magTMP1(dimensionN, dimensionN, CV_64F, current3DCorrelation);
//    cv::GaussianBlur(magTMP1, magTMP1, cv::Size(9, 9), 0);

//    cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE,
//                                            cv::Size(ceil(0.02 * this->correlationN), ceil(0.02 * this->correlationN)));
//    cv::morphologyEx(magTMP1, magTMP1, cv::MORPH_TOPHAT, element);

    size_t ourSize = dimensionN;
    findpeaks::volume4D_t<double> volume = {
            ourSize, ourSize, ourSize, ourSize,
            current4DCorrelation
    };

    std::vector<findpeaks::peak_4d<double>> peaks = findpeaks::persistance4d(volume);
    std::vector<rotationPeak4D> tmpRotations;
    std::cout << peaks.size() << std::endl;
    int numberOfPeaks = 0;
    for (const auto &p: peaks) {
        //calculation of level, that is a potential translation
        double levelPotential = p.persistence * sqrt(p.birth_level) *
                                Eigen::Vector4d((double) ((int) p.birth_position.x - (int) p.death_position.x),
                                                (double) ((int) p.birth_position.y - (int) p.death_position.y),
                                                (double) ((int) p.birth_position.z - (int) p.death_position.z),
                                                (double) ((int) p.birth_position.w - (int) p.death_position.w)).norm() /
                                dimensionN / 1.73205080757;

//        std::cout << p.persistence << std::endl;
//        std::cout << Eigen::Vector4d((double) ((int) p.birth_position.x - (int) p.death_position.x),
//                                     (double) ((int) p.birth_position.y - (int) p.death_position.y),
//                                     (double) ((int) p.birth_position.z - (int) p.death_position.z),
//                                     (double) ((int) p.birth_position.w - (int) p.death_position.w)).norm()
//                  << std::endl;
//        std::cout << sqrt(p.birth_level) << std::endl;


        rotationPeak4D tmpTranslationPeak;
        tmpTranslationPeak.x = p.birth_position.x;
        tmpTranslationPeak.y = p.birth_position.y;
        tmpTranslationPeak.z = p.birth_position.z;
        tmpTranslationPeak.w = p.birth_position.w;
        tmpTranslationPeak.persistence = p.persistence;
        tmpTranslationPeak.correlationHeight = current4DCorrelation[p.birth_position.w +
                                                                    dimensionN * p.birth_position.z +
                                                                    p.birth_position.y * dimensionN *
                                                                    dimensionN +
                                                                    p.birth_position.x * dimensionN * dimensionN *
                                                                    dimensionN];
        tmpTranslationPeak.levelPotential = levelPotential;
        if (levelPotential > 0.01) {
            tmpRotations.push_back(tmpTranslationPeak);
//            std::cout << "test" << std::endl;
            numberOfPeaks++;
        }


    }
    free(current4DCorrelation);
    return (tmpRotations);

}

std::vector<rotationPeak4D>
softRegistrationClass3D::peakDetectionOf4DCorrelationWithKDTreeFindPeaksLibrary(
        std::vector<My4DPoint> listOfQuaternionCorrelation) {

    double *current1DCorrelation;
    current1DCorrelation = (double *) malloc(
            sizeof(double) * listOfQuaternionCorrelation.size());

    for (int i = 0; i < listOfQuaternionCorrelation.size(); i++) {
        current1DCorrelation[i] = listOfQuaternionCorrelation[i].correlation;
    }


    findpeaks::oneDimensionalList_t<double> inputCorrelations = {
            listOfQuaternionCorrelation.size(),
            current1DCorrelation
    };
    kdt::KDTree<My4DPoint> kdtree(listOfQuaternionCorrelation);

    std::vector<findpeaks::peak_1d<double>> peaks = findpeaks::persistanceQuaternionsKDTree(inputCorrelations, kdtree);
    std::vector<rotationPeak4D> tmpRotations;

    int numberOfPeaks = 0;
    for (const auto &p: peaks) {
        My4DPoint birthPositionPoint = listOfQuaternionCorrelation[p.birth_position.x];
        My4DPoint deathPositionPoint = listOfQuaternionCorrelation[p.death_position.x];
        double currentPersistence;
        if (p.persistence == INFINITY) {
            currentPersistence = 1;
        } else {
            currentPersistence = p.persistence;
        }
        double levelPotential = currentPersistence * p.birth_level * p.birth_level;

        rotationPeak4D tmpTranslationPeak;
        tmpTranslationPeak.x = birthPositionPoint[1];
        tmpTranslationPeak.y = birthPositionPoint[2];
        tmpTranslationPeak.z = birthPositionPoint[3];
        tmpTranslationPeak.w = birthPositionPoint[0];
        tmpTranslationPeak.persistence = p.persistence;
        tmpTranslationPeak.correlationHeight = birthPositionPoint.correlation;
        tmpTranslationPeak.levelPotential = levelPotential;
        if (levelPotential > 0.1) {
            tmpRotations.push_back(tmpTranslationPeak);
//            std::cout << "test" << std::endl;
            numberOfPeaks++;
        }


    }
    free(current1DCorrelation);
    return (tmpRotations);

}