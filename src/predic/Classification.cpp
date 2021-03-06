//
// Created by maiikeru on 3/17/16.
//

#include <opencv2/imgcodecs.hpp>
#include "Classification.h"

Classification::Classification(string model_file) {
    this->model_file_name = model_file;


    this->LoadImagesIcon();

}

bool Classification::LoadImagesIcon() {


    vector<string> list;

    boost::filesystem::path path = "/home/maiikeru/bitbucket/BP-road-sign-detection/data_sets/icons/";
    boost::filesystem::recursive_directory_iterator itr(path);
    while (itr != boost::filesystem::recursive_directory_iterator()) {
        list.push_back(itr->path().string());
        ++itr;
    }


    for (int i = 0; i < list.size(); ++i) {
        this->images_icon.push_back(cv::imread(LUT_image_filename[i]));
    }


    return false;
}

vector<cv::Mat> Classification::GetImgIcon() {
    return this->images_icon;
}

bool Classification::CheckRoi(int i, int i1) {

    return i - i1 > 0;

}

Classification::~Classification() {
    freeMem();

    delete this->predic_prob;
}

void Classification::freeMem() {
    libsvm::svm_destroy_param(&this->param);
    libsvm::svm_free_and_destroy_model(&model);
}

libsvm::svm_model *Classification::loadModelFromFile(bool *pBoolean) {

    cout << "QQ" << endl;
    //this->freeMem();
    setlocale(LC_NUMERIC, "C");
    setlocale(LC_ALL, "POSIX");
    if (this->usedModel && model != NULL) {
        delete model; // clear the previous model and deallocate/free the occupied memory
        model = NULL; // set the pointer to NULL
    }
    printf("Loading model from file '%s'\n", this->model_file_name.c_str());
    model = libsvm::svm_load_model(this->model_file_name.c_str());
    if (model == NULL) {
        printf("Loading of SVM model failed!\n");
        *pBoolean = false;
    }


    int classNr = svm_get_nr_class(model);
    int labels[classNr];

    this->classesN = classNr;

    for (int i = 0; i < classNr; ++i) {
        labels[i] = 0;
    }
    svm_get_labels(model, labels);

    for (int i = 0; i < classNr; ++i) {
        this->label_model.push_back(fabs(labels[i]));
    }


    double b = -(model->rho[0]);
    //double probA = (model->probA[0]);
    double probA = 0.0;
    //double probB = (model->probB[0]);
    double probB = 0.0;
    int kernelType = model->param.kernel_type;
    printf("Loaded model: SVM type %d, Kernel type %d, %d classes: labels %d, %d, #SVs %d, bias b %3.5f, probA %3.5f, probB %3.5f\n",
           svm_get_svm_type(model), kernelType, classNr, labels[0], labels[1], model->l, b, probA, probB);
    this->usedModel = true;
    return model;
}

double Classification::detekuj_prob(vector<float> value) {

    struct libsvm::svm_node *svmVecT;
    svmVecT = (struct libsvm::svm_node *) malloc((value.size() + 1) * sizeof(struct libsvm::svm_node));

    int i;
    for (i = 0; i < value.size(); i++) {
        svmVecT[i].index = i + 1;
        svmVecT[i].value = value.at(i);
    }

    svmVecT[i].index = -1;   // End of line


    this->predic_prob = new double[this->classesN];


    double td = (double) cv::getTickCount();
    double result = svm_predict_probability(this->model, svmVecT, this->predic_prob);
    td = ((double) cv::getTickCount() - td) / cv::getTickFrequency();
    cout << td << endl;

    ////sort(prob, prob+45);
    int j = 0;

    for (; j < this->label_model.size(); ++j) {

        if ((fabs(result)) == this->label_model.at(j)) {
            //cout << "Q:" << j << endl;
            this->predic_index = j;
            break;
        }
    }


    //cerr << result << "W:" << j << ":" << this->predic_prob[j] << endl;


    free(svmVecT);
    return (fabs(result));
}

vector<float> Classification::extractHog(cv::Rect_<int> &faces, cv::Mat mat) {
    cv::Mat cropedImage = mat(cv::Rect(faces.x, faces.y, faces.width, faces.height));

    std::vector<float> descriptors;
    descriptors = getHog(cropedImage);
    //descriptors = getHog(mat);
    cropedImage.release();

    return descriptors;
}

vector<float> Classification::extractHog(cv::Mat mat) {

    return getHog(mat);
}


bool Classification::loadModel() {

    bool error = true;

    cout << "A" << endl;
    this->model = this->loadModelFromFile(&error);

    cout << "B" << endl;

    return error;
}

vector<double> Classification::predic(cv::Mat frame, cv::Mat original, vector<cv::Rect> sign) {


    double dw, dh;

    //std::vector<cv::Rect>  object;


    dw = (double) original.rows / (double) frame.rows;
    dh = (double) original.cols / (double) frame.cols;

    std::vector<cv::Rect> obj;
    for (int j = 0; j < sign.size(); ++j) {

        obj = sign;

        obj.at(j).x *= dh;
        obj.at(j).y *= dw;
        obj.at(j).width *= dh;
        obj.at(j).height *= dw;


        std::vector<float> descriptors = this->extractHog(obj.at(j), original);
        double index = this->detekuj_prob(descriptors);

        if (this->predic_prob[this->predic_index] >= 0.2) {
            this->object.push_back(sign.at(j));
            predic_classes.push_back(index);


            rectangle(frame, sign.at(j), cv::Scalar(0, 255, 0), 2, 6, 0);

            if (this->CheckRoi(sign.at(j).x, sign.at(j).height)) {
                cv::Mat small_image = this->images_icon.at(abs((int) index) - 1);
                resize(small_image, small_image, cv::Size(sign.at(j).height, sign.at(j).width), cv::INTER_CUBIC);
                small_image.copyTo(
                        frame(cv::Rect(sign.at(j).x - sign.at(j).height, sign.at(j).y, small_image.cols,
                                       small_image.rows)));
                small_image.release();
            }


        }


        cout << "\nMAX prob:     " << this->predic_prob[this->predic_index] << endl;


        cout << index << endl;


    }


    return predic_classes;
}


vector<double> Classification::predicCross(cv::Mat original, vector<cv::Rect> sign, string roc_file, string msr_file,
                                           bool b) {
    std::vector<double> predic_classes;
    unsigned long item;

    if (b)
        item = 1;
    else
        item = sign.size();

    // Write result to files
    ofstream f_c_roc, f_c_msr;
    f_c_roc.open(roc_file, ios::app);
    f_c_msr.open(msr_file, ios::app);


    if (item > 1)
        item = 1;

    for (int j = 0; j < item; ++j) {

        // Extract hog
        std::vector<float> descriptors;
        if (b)
            descriptors = this->extractHog(original);
        else
            descriptors = this->extractHog(sign.at(j), original);

        //Prediction
        double index = this->detekuj_prob(descriptors);

        predic_classes.push_back(index);

        f_c_msr << index << endl;
        f_c_roc << index << " ";
        for (int i = 0; i < this->classesN; ++i) {
            f_c_roc << this->predic_prob[i] << " ";
        }
        f_c_roc << "\n";

    }

    if (sign.empty()) {
        f_c_msr << "0" << endl;
    }


    // Close files
    f_c_msr.close();
    f_c_roc.close();

    return predic_classes;
}

void Classification::WriteLabel(string filename) {

    ofstream file;
    file.open(filename, ios::out);

    // Write labels
    if (file.good()) {
        file << "labels ";

        for (int i = 0; i < this->label_model.size(); ++i) {
            file << this->label_model.at(i) << " ";
        }
        file << "\n";
    }

    file.close();

}

void Classification::DrawClassif(cv::Mat mat) {


    for (int j = 0; j < this->object.size(); j++) {
        rectangle(mat, this->object.at(j), cv::Scalar(0, 255, 0), 2, 6, 0);

        if (this->CheckRoi(this->object.at(j).x, this->object.at(j).height)) {
            cv::Mat small_image = this->images_icon.at(abs((int) this->predic_classes.at(j)) - 1);
            resize(small_image, small_image, cv::Size(this->object.at(j).height, this->object.at(j).width),
                   cv::INTER_CUBIC);
            small_image.copyTo(
                    mat(cv::Rect(this->object.at(j).x - this->object.at(j).height, this->object.at(j).y,
                                 small_image.cols,
                                 small_image.rows)));
            small_image.release();
        }


    }


}
