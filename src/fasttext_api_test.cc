#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <cstdio>
#include "fasttext_api.h"

using std::cout;
using std::endl;
using std::string;
using std::ofstream;

TestMetrics* FindMetrics(TestMeter* meter, int label);
bool file_exists(const char *filename);
bool vector_has_nonzero_elements(const float* vector, int size);

TEST_CASE("Struct sizes are correct")
{
    REQUIRE(sizeof(TrainingArgs) == 100);
    REQUIRE(sizeof(AutotuneArgs) == 36);
    REQUIRE(sizeof(TestMetrics) == 48);
    REQUIRE(sizeof(TestMeter) == 40);
}

TEST_CASE("Can get dimension on empty model")
{
    auto hPtr = CreateFastText();

    int dim = GetModelDimension(hPtr);
    REQUIRE(dim == 0);

    DestroyFastText(hPtr);
}

TEST_CASE("Empty model is not ready")
{
    auto hPtr = CreateFastText();

    REQUIRE_FALSE(IsModelReady(hPtr));

    DestroyFastText(hPtr);
}

TEST_CASE("Can handle errors")
{
    auto hPtr = CreateFastText();
    TrainingArgs* args;

    GetDefaultSupervisedArgs(&args);

    int result = Train(hPtr, "not/a/valid.file", "tests/models/test", *args, AutotuneArgs(), nullptr, nullptr, nullptr, nullptr, false);
    REQUIRE(result == -1);

    char* buff;
    GetLastErrorText(&buff);
    REQUIRE(std::string(buff) == "not/a/valid.file cannot be opened for training!");

    DestroyString(buff);
    DestroyArgs(args);
    DestroyFastText(hPtr);
}

TEST_CASE("Can train unsupervised model")
{
    std::remove("tests/models/test.bin");
    std::remove("tests/models/test.vec");

    REQUIRE_FALSE(file_exists("tests/models/test.bin"));
    REQUIRE_FALSE(file_exists("tests/models/test.vec"));

    auto hPtr = CreateFastText();
    TrainingArgs* args;

    GetDefaultArgs(&args);
    int result = Train(hPtr, "tests/cooking/cooking.train.nolabels.txt", "tests/models/test", *args, AutotuneArgs(), nullptr, nullptr, nullptr, nullptr, false);

    REQUIRE(result == 0);
    REQUIRE(IsModelReady(hPtr));
    REQUIRE(GetModelDimension(hPtr) == 100);

    DestroyArgs(args);
    DestroyFastText(hPtr);

    REQUIRE(file_exists("tests/models/test.bin"));
    REQUIRE(file_exists("tests/models/test.vec"));

    std::remove("tests/models/test.bin");
    std::remove("tests/models/test.vec");
}

TEST_CASE("Can autotune quantized supervised model with callback")
{
    std::remove("tests/models/test.bin");
    std::remove("tests/models/test.ftz");
    std::remove("tests/models/test.vec");
    std::remove("tests/models/callback.txt");
    std::remove("_train.txt");

    REQUIRE_FALSE(file_exists("tests/models/test.bin"));
    REQUIRE_FALSE(file_exists("tests/models/test.vec"));
    REQUIRE_FALSE(file_exists("tests/models/callback.txt"));
    REQUIRE_FALSE(file_exists("_train.txt"));

    auto hPtr = CreateFastText();
    TrainingArgs* args;
    AutotuneArgs tuneArgs;

    tuneArgs.validationFile = "tests/cooking/cooking.valid.txt";
    tuneArgs.duration = 30;
    tuneArgs.modelSize = "10M";

    AutotuneProgressCallback callback = [](double progress, int32_t trials, double bestScore, double eta) {
      ofstream stream("tests/models/callback.txt", ofstream::out | ofstream::app);
      stream << progress << ";" << trials << ";" << bestScore << ";" << eta << endl;
      stream.close();
    };

    GetDefaultSupervisedArgs(&args);
    int result = Train(hPtr, "tests/cooking/cooking.train.txt", "tests/models/test", *args, tuneArgs, nullptr, callback, nullptr, nullptr, true);

    REQUIRE(result == 0);
    REQUIRE(IsModelReady(hPtr));
    REQUIRE(GetModelDimension(hPtr) == 100);

    DestroyArgs(args);
    DestroyFastText(hPtr);

    REQUIRE(file_exists("tests/models/test.ftz"));
    REQUIRE(file_exists("tests/models/test.vec"));
    REQUIRE(file_exists("tests/models/callback.txt"));
    REQUIRE(file_exists("_train.txt"));

    std::remove("tests/models/test.bin");
    std::remove("tests/models/test.ftz");
    std::remove("tests/models/test.vec");
    std::remove("tests/models/callback.txt");
    std::remove("_train.txt");
}

TEST_CASE("Can autotune quantized supervised model")
{
    std::remove("tests/models/test.bin");
    std::remove("tests/models/test.ftz");
    std::remove("tests/models/test.vec");
    std::remove("_train.txt");

    REQUIRE_FALSE(file_exists("tests/models/test.bin"));
    REQUIRE_FALSE(file_exists("tests/models/test.vec"));
    REQUIRE_FALSE(file_exists("_train.txt"));

    auto hPtr = CreateFastText();
    TrainingArgs* args;
    AutotuneArgs tuneArgs;

    tuneArgs.validationFile = "tests/cooking/cooking.valid.txt";
    tuneArgs.duration = 30;
    tuneArgs.modelSize = "10M";

    GetDefaultSupervisedArgs(&args);
    int result = Train(hPtr, "tests/cooking/cooking.train.txt", "tests/models/test", *args, tuneArgs, nullptr, nullptr, nullptr, nullptr, true);

    REQUIRE(result == 0);
    REQUIRE(IsModelReady(hPtr));
    REQUIRE(GetModelDimension(hPtr) == 100);

    DestroyArgs(args);
    DestroyFastText(hPtr);

    REQUIRE(file_exists("tests/models/test.ftz"));
    REQUIRE(file_exists("tests/models/test.vec"));
    REQUIRE(file_exists("_train.txt"));

    std::remove("tests/models/test.bin");
    std::remove("tests/models/test.ftz");
    std::remove("tests/models/test.vec");
    std::remove("_train.txt");
}

TEST_CASE("Can train and quantize supervised model")
{
    std::remove("tests/models/test.bin");
    std::remove("tests/models/test.ftz");
    std::remove("tests/models/test.vec");

    REQUIRE_FALSE(file_exists("tests/models/test.bin"));
    REQUIRE_FALSE(file_exists("tests/models/test.ftz"));
    REQUIRE_FALSE(file_exists("tests/models/test.vec"));

    auto hPtr = CreateFastText();
    TrainingArgs* args;

    GetDefaultSupervisedArgs(&args);
    int result = Train(hPtr, "tests/cooking/cooking.train.txt", "tests/models/test", *args, AutotuneArgs(), nullptr, nullptr, nullptr, nullptr, false);

    REQUIRE(result == 0);
    REQUIRE(IsModelReady(hPtr));
    REQUIRE(GetModelDimension(hPtr) == 100);

    REQUIRE(file_exists("tests/models/test.bin"));
    REQUIRE(file_exists("tests/models/test.vec"));

    result = Quantize(hPtr, "tests/models/test", *args, nullptr);

    REQUIRE(result == 0);
    REQUIRE(IsModelReady(hPtr));
    REQUIRE(GetModelDimension(hPtr) == 100);

    DestroyArgs(args);
    DestroyFastText(hPtr);

    REQUIRE(file_exists("tests/models/test.ftz"));

    std::remove("tests/models/test.bin");
    std::remove("tests/models/test.ftz");
    std::remove("tests/models/test.vec");
}

TEST_CASE("Can train supervised model without saving")
{
    std::remove("tests/models/test.bin");
    std::remove("tests/models/test.ftz");
    std::remove("tests/models/test.vec");

    REQUIRE_FALSE(file_exists("tests/models/test.bin"));
    REQUIRE_FALSE(file_exists("tests/models/test.ftz"));
    REQUIRE_FALSE(file_exists("tests/models/test.vec"));

    auto hPtr = CreateFastText();
    TrainingArgs* args;

    GetDefaultSupervisedArgs(&args);
    int result = Train(hPtr, "tests/cooking/cooking.train.txt", nullptr, *args, AutotuneArgs(), nullptr, nullptr, nullptr, nullptr, false);

    REQUIRE(result == 0);
    REQUIRE(IsModelReady(hPtr));
    REQUIRE(GetModelDimension(hPtr) == 100);

    REQUIRE_FALSE(file_exists("tests/models/test.bin"));
    REQUIRE_FALSE(file_exists("tests/models/test.vec"));

    DestroyArgs(args);
    DestroyFastText(hPtr);

    std::remove("tests/models/test.bin");
    std::remove("tests/models/test.ftz");
    std::remove("tests/models/test.vec");
}

TEST_CASE("Can train supervised model with callback")
{
    std::remove("tests/models/test.bin");
    std::remove("tests/models/test.ftz");
    std::remove("tests/models/test.vec");
    std::remove("tests/models/callback.txt");

    REQUIRE_FALSE(file_exists("tests/models/test.bin"));
    REQUIRE_FALSE(file_exists("tests/models/test.ftz"));
    REQUIRE_FALSE(file_exists("tests/models/test.vec"));
    REQUIRE_FALSE(file_exists("tests/models/callback.txt"));

    auto hPtr = CreateFastText();
    TrainingArgs* args;

    int callCnt = 0;
    TrainProgressCallback callback = [](float progress, float loss, double wst, double lr, int64_t eta) {
      ofstream stream("tests/models/callback.txt", ofstream::out | ofstream::app);
      stream << progress << ";" << loss << ";" << wst << ";" << lr << ";" << eta << endl;
      stream.close();
    };

    GetDefaultSupervisedArgs(&args);
    int result = Train(hPtr, "tests/cooking/cooking.train.txt", nullptr, *args, AutotuneArgs(), callback, nullptr, nullptr, nullptr, false);

    REQUIRE(result == 0);
    REQUIRE(IsModelReady(hPtr));
    REQUIRE(GetModelDimension(hPtr) == 100);

    REQUIRE_FALSE(file_exists("tests/models/test.bin"));
    REQUIRE_FALSE(file_exists("tests/models/test.vec"));
    REQUIRE(file_exists("tests/models/callback.txt"));

    DestroyArgs(args);
    DestroyFastText(hPtr);

    std::remove("tests/models/test.bin");
    std::remove("tests/models/test.ftz");
    std::remove("tests/models/test.vec");
    std::remove("tests/models/callback.txt");
}

TEST_CASE("Can load model")
{
    std::remove("tests/models/test.bin");
    std::remove("tests/models/test.ftz");
    std::remove("tests/models/test.vec");

    REQUIRE_FALSE(file_exists("tests/models/test.bin"));
    REQUIRE_FALSE(file_exists("tests/models/test.vec"));

    auto hPtr = CreateFastText();
    TrainingArgs* args;

    GetDefaultSupervisedArgs(&args);
    int result = Train(hPtr, "tests/cooking/cooking.train.txt", "tests/models/test", *args, AutotuneArgs(), nullptr, nullptr, nullptr, nullptr, false);

    REQUIRE(result == 0);
    REQUIRE(IsModelReady(hPtr));
    REQUIRE(GetModelDimension(hPtr) == 100);

    REQUIRE(file_exists("tests/models/test.bin"));
    REQUIRE(file_exists("tests/models/test.vec"));

    DestroyArgs(args);
    DestroyFastText(hPtr);

    hPtr = CreateFastText();

    LoadModel(hPtr, "tests/models/test.bin");

    REQUIRE(IsModelReady(hPtr));
    REQUIRE(GetModelDimension(hPtr) == 100);

    DestroyFastText(hPtr);
    std::remove("tests/models/test.bin");
    std::remove("tests/models/test.vec");
}

TEST_CASE("Can perform operations on a model")
{
    SECTION("Can train supervised model")
    {
        std::remove("tests/models/test.bin");
        std::remove("tests/models/test.vec");

        REQUIRE_FALSE(file_exists("tests/models/test.bin"));
        REQUIRE_FALSE(file_exists("tests/models/test.vec"));

        auto hPtr = CreateFastText();
        TrainingArgs* args;

        GetDefaultSupervisedArgs(&args);
        int result = Train(hPtr, "tests/cooking/cooking.train.txt", "tests/models/test", *args, AutotuneArgs(), nullptr, nullptr, nullptr, nullptr, false);

        REQUIRE(result == 0);
        REQUIRE(IsModelReady(hPtr));
        REQUIRE(GetModelDimension(hPtr) == 100);

        REQUIRE(file_exists("tests/models/test.bin"));
        REQUIRE(file_exists("tests/models/test.vec"));

        SECTION("Can get sentence vector")
        {
            float* vector;
            int dim = GetSentenceVector(hPtr, "what is the difference between a new york strip and a bone-in new york cut sirloin", &vector);

            REQUIRE(dim == 100);
            REQUIRE(vector_has_nonzero_elements(vector, dim));

            DestroyVector(vector);
        }

        SECTION("Can get word vector")
        {
            float* vector;
            int dim = GetWordVector(hPtr, "pot", &vector);

            REQUIRE(dim == 100);
            REQUIRE(vector_has_nonzero_elements(vector, dim));

            DestroyVector(vector);
        }

        SECTION("Can get model labels")
        {
            char** labels;
            int nLabels = GetLabels(hPtr, &labels);

            REQUIRE(nLabels == 735);

            for (int i = 0; i<nLabels; ++i) {
                REQUIRE(!string(labels[i]).empty());
            }

            DestroyStrings(labels, nLabels);
        }

        SECTION("Can predict single label")
        {
            char* buff;
	        float prob = PredictSingle(hPtr, "what is the difference between a new york strip and a bone-in new york cut sirloin ?", &buff);

	        REQUIRE(prob > 0);
	        REQUIRE(!string(buff).empty());

	        DestroyString(buff);
        }

        SECTION("Can predict multiple labels")
        {
            char** buffers;
            float* probs = new float[5];

            int cnt = PredictMultiple(hPtr,"what is the difference between a new york strip and a bone-in new york cut sirloin ?", &buffers, probs, 5);

            REQUIRE(cnt == 5);

            for (int i = 0; i<cnt; ++i) {
                REQUIRE(!string(buffers[i]).empty());
                REQUIRE(probs[i] > 0);
            }

            DestroyStrings(buffers, 5);
        }

        SECTION("Can get nearest neighbours")
        {
            char** buffers;
            float* probs = new float[5];

            int cnt = GetNN(hPtr, "train", &buffers, probs, 5);

            REQUIRE(cnt == 5);

            for (int i = 0; i<cnt; ++i) {
                REQUIRE(!string(buffers[i]).empty());
                REQUIRE(probs[i] > 0);
            }

            DestroyStrings(buffers, 5);
        }

        SECTION("Can get sentence vector")
        {
            float* vector;

            int len = GetSentenceVector(hPtr, "This is only a test!", &vector);

            REQUIRE(len == 100);
            REQUIRE(vector_has_nonzero_elements(vector, len));

            DestroyVector(vector);
        }

        SECTION("Can test a model")
        {
            TestMeter* meterPtr = nullptr;

            int res = Test(hPtr, "tests/cooking/cooking.valid.txt", 1, 0.0, &meterPtr, false);

            REQUIRE(res == 0);
            REQUIRE(meterPtr != nullptr);
            REQUIRE(meterPtr->nlabels == 628);
            REQUIRE(meterPtr->nexamples == 3000);

            REQUIRE(meterPtr->metrics->gold == meterPtr->sourceMeter->metrics_.gold);
            REQUIRE(meterPtr->metrics->predictedGold == meterPtr->sourceMeter->metrics_.predictedGold);
            REQUIRE(meterPtr->metrics->predicted == meterPtr->sourceMeter->metrics_.predicted);

            for(auto& srcMetrics : meterPtr->sourceMeter->labelMetrics_)
            {
                auto metrics = FindMetrics(meterPtr, srcMetrics.first);

                REQUIRE(metrics !=nullptr);
                REQUIRE(metrics->scoresLen == srcMetrics.second.scoreVsTrue.size());
                REQUIRE(metrics->gold == srcMetrics.second.gold);
                REQUIRE(metrics->predicted == srcMetrics.second.predicted);
                REQUIRE(metrics->predictedGold == srcMetrics.second.predictedGold);

                if (!srcMetrics.second.scoreVsTrue.empty())
                {
                    //cout << "Checking loop for label " << srcMetrics.first << " with " << srcMetrics.second.scoreVsTrue.size() << " items." << std::endl;
                    for (int i = 0; i < srcMetrics.second.scoreVsTrue.size(); ++i)
                    {
                        REQUIRE(metrics->goldScores[i] == srcMetrics.second.scoreVsTrue[i].second);
                        REQUIRE(metrics->predictedScores[i] == srcMetrics.second.scoreVsTrue[i].first);
                    }
                }
            }

            DestroyMeter(meterPtr);
        }

        DestroyFastText(hPtr);

        std::remove("tests/models/test.bin");
        std::remove("tests/models/test.vec");
    }
}

TestMetrics* FindMetrics(TestMeter* meter, int label)
{
    for (int i = 0; i<meter->nlabels; ++i) {
        if (meter->labelMetrics[i]->label == label)
            return meter->labelMetrics[i];
    }

    return nullptr;
}

bool vector_has_nonzero_elements(const float* vector, int size)
{
    for (int i = 0; i<size; ++i) {
        if (vector[i] != 0)
            return true;
    }

    return false;
}

bool file_exists(const char *filename)
{
    return static_cast<bool>(std::ifstream(filename));
}
