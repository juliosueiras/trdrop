#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickView>
#include <QQmlContext>
#include <QFontDatabase>
//#include <QQmlDebuggingEnabler>
#include <QCommandLineParser>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "headers/qml_models/fileitemmodel.h"
#include "headers/qml_models/generaloptionsmodel.h"
#include "headers/qml_models/framerateoptionsmodel.h"
#include "headers/qml_models/tearoptionsmodel.h"
#include "headers/qml_models/resolutionsmodel.h"
#include "headers/qml_models/imageformatmodel.h"
#include "headers/qml_models/exportoptionsmodel.h"

#include "headers/qml_interface/videocapturelist_qml.h"
#include "headers/qml_interface/imageconverter_qml.h"
#include "headers/qml_interface/imagecomposer_qml.h"
#include "headers/qml_interface/frameprocessing_qml.h"
#include "headers/qml_interface/renderer_qml.h"
#include "headers/qml_interface/viewer_qml.h"
#include "headers/qml_interface/exporter_qml.h"

#include "headers/cpp_interface/frameratemodel.h"
#include "headers/cpp_interface/frametimemodel.h"
#include "headers/cpp_interface/framerateplot.h"
#include "headers/cpp_interface/frametimeplot.h"

int main(int argc, char *argv[])
{
    // general application stuff
    //QQmlDebuggingEnabler enabler;
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setApplicationName("trdrop");
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("trdrop - command line version");
    parser.addHelpOption();
    parser.addVersionOption();

    // Add custom options
    parser.addPositionalArgument("source video", QCoreApplication::translate("main", "Source video file"));
    parser.addPositionalArgument("destination directory", QCoreApplication::translate("main", "Destination directory"));
    //
    parser.addOption(QCommandLineOption{{"n", "image-prefix"}, QCoreApplication::translate("main", "image prefix name"), QCoreApplication::translate("main", "image prefix name")});

    parser.process(app);

    if (parser.isSet("help")) {
        parser.showHelp();
        return 0;
    }

    QQmlApplicationEngine engine;
    app.setOrganizationName("trdrop");
    app.setOrganizationDomain("trdrop");
    QQuickStyle::setStyle("Material");
    QFontDatabase::addApplicationFont(":/fonts/FjallaOne-Regular.ttf");
    // TODO evaluate if we need these fonts
    QFontDatabase::addApplicationFont(":/fonts/materialdesignicons-webfont.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Rationale-Regular.ttf");

    // c++ models
    std::shared_ptr<FramerateModel> shared_framerate_model(new FramerateModel());
    std::shared_ptr<FrametimeModel> shared_frametime_model(new FrametimeModel());
    std::shared_ptr<QList<FramerateOptions>> shared_fps_options_list(new QList<FramerateOptions>());
    std::shared_ptr<QList<TearOptions>> shared_tear_options_list(new QList<TearOptions>());

    // qml models
    // prepare the FileItemModel
    qmlRegisterType<FileItemModel>();
    constexpr quint8 default_file_items_count = 3;
    FileItemModel file_item_model(default_file_items_count);
    engine.rootContext()->setContextProperty("fileItemModel", &file_item_model);

    // prepare the Tear Options Model
    qmlRegisterType<TearOptionsModel>();
    TearOptionsModel tear_options_model(shared_tear_options_list);
    engine.rootContext()->setContextProperty("tearOptionsModel", &tear_options_model);

    // prepare the OptionsModel
    qmlRegisterType<GeneralOptionsModel>();
    std::shared_ptr<GeneralOptionsModel> shared_general_options_model(new GeneralOptionsModel());
    engine.rootContext()->setContextProperty("generalOptionsModel", &(*shared_general_options_model));

    // prepare ResolutionsModel (Exporter)
    qmlRegisterType<ResolutionsModel>();
    std::shared_ptr<ResolutionsModel> shared_resolution_model(new ResolutionsModel());
    engine.rootContext()->setContextProperty("resolutionsModel", &(*shared_resolution_model));

    // prepare the FPS Options Model
    qmlRegisterType<FramerateOptionsModel>();
    FramerateOptionsModel framerate_options_model(shared_framerate_model, shared_fps_options_list, shared_resolution_model);
    engine.rootContext()->setContextProperty("framerateOptionsModel", &framerate_options_model);

    // prepare ImagFormatModel (Exporter)
    qmlRegisterType<ImageFormatModel>();
    std::shared_ptr<ImageFormatModel> shared_imageformat_model(new ImageFormatModel());
    engine.rootContext()->setContextProperty("imageFormatModel", &(*shared_imageformat_model));

    // prepare ExportOptionsModel (Exporter)
    qmlRegisterType<ExportOptionsModel>();
    //ExportOptionsModel export_options_model;
    std::shared_ptr<ExportOptionsModel> shared_export_options_model(new ExportOptionsModel());
    engine.rootContext()->setContextProperty("exportOptionsModel", &(*shared_export_options_model));

    // allow cv::Mat in signals
    qRegisterMetaType<cv::Mat>("cv::Mat");
    // allow const QList<FPSOptions> in signals
    qRegisterMetaType<QList<FramerateOptions>>("const QList<FPSOptions>");
    // allow const QList<quint8> in signals
    qRegisterMetaType<QList<quint8>>("const QList<quint8>");

    // register the viewer as qml type
    qmlRegisterType<ViewerQML>("Trdrop", 1, 0, "ViewerQML");

    // c++ plots
    std::shared_ptr<FrameratePlot> shared_framerate_plot_instance(new FrameratePlot(shared_framerate_model
                                                      , shared_fps_options_list
                                                      , shared_resolution_model
                                                      , shared_general_options_model));
    std::shared_ptr<FrametimePlot> shared_frametime_plot_instance(new FrametimePlot(shared_frametime_model
                                                      , shared_fps_options_list
                                                      , shared_resolution_model
                                                      , shared_general_options_model));
    std::shared_ptr<TearModel> shared_tear_model(new TearModel(shared_tear_options_list
                                                             , shared_resolution_model
                                                             , shared_general_options_model));
    // qml objects
    VideoCaptureListQML videocapturelist_qml(default_file_items_count);
    engine.rootContext()->setContextProperty("videocapturelist", &videocapturelist_qml);
    FrameProcessingQML frame_processing_qml(shared_framerate_model
                                          , shared_frametime_model
                                          , shared_fps_options_list
                                          , shared_tear_options_list
                                          , shared_general_options_model
                                          , shared_tear_model);
    engine.rootContext()->setContextProperty("frameprocessing", &frame_processing_qml);
    ImageConverterQML imageconverter_qml;
    engine.rootContext()->setContextProperty("imageconverter", &imageconverter_qml);
    ImageComposerQML imagecomposer_qml(shared_resolution_model);
    engine.rootContext()->setContextProperty("imagecomposer", &imagecomposer_qml);
    RendererQML renderer_qml(shared_fps_options_list
                           , shared_general_options_model
                           , shared_export_options_model
                           , shared_framerate_plot_instance
                           , shared_frametime_plot_instance
                           , shared_tear_model
                           , shared_resolution_model);
    engine.rootContext()->setContextProperty("renderer", &renderer_qml);
    ExporterQML exporter_qml(shared_export_options_model
                           , shared_imageformat_model
                           , shared_framerate_model
                           , shared_frametime_model);
    engine.rootContext()->setContextProperty("exporter", &exporter_qml);

    // sigals in c++ (main processing pipeline)
    // pass the QList<cv::Mat> to the converter
    QObject::connect(&videocapturelist_qml, &VideoCaptureListQML::framesReady, &frame_processing_qml, &FrameProcessingQML::processFrames, Qt::DirectConnection);
    // framerate processing
    QObject::connect(&frame_processing_qml, &FrameProcessingQML::framesReady,  &imageconverter_qml,   &ImageConverterQML::processFrames, Qt::DirectConnection);
    // pass the QList<QImage> to the composer to mux them together
    QObject::connect(&imageconverter_qml,   &ImageConverterQML::imagesReady,   &imagecomposer_qml,    &ImageComposerQML::processImages, Qt::DirectConnection);
    // pass the QImage to the renderer to render the meta information onto the image
    QObject::connect(&imagecomposer_qml,    &ImageComposerQML::imageReady,     &renderer_qml,         &RendererQML::processImage, Qt::DirectConnection);
    // pass the rendered QImage to the exporter
    QObject::connect(&renderer_qml,         &RendererQML::imageReady,          &exporter_qml,         &ExporterQML::processImage, Qt::DirectConnection);

    // if VCL finishes processing, finish exporting (close io-handles if need be)
    QObject::connect(&videocapturelist_qml, &VideoCaptureListQML::finishedProcessing, &exporter_qml, &ExporterQML::finishExporting);

    // the exporter may trigger a request for new frames from VCL
    QObject::connect(&exporter_qml,         &ExporterQML::requestNextImages,   &videocapturelist_qml, &VideoCaptureListQML::readNextFrames);

    // meta data pipeline
    QObject::connect(&framerate_options_model,         &FramerateOptionsModel::dataChanged, &renderer_qml, &RendererQML::redraw);
    QObject::connect(&tear_options_model,              &TearOptionsModel::dataChanged,      &renderer_qml, &RendererQML::redraw);
    QObject::connect(&(*shared_general_options_model), &GeneralOptionsModel::dataChanged,   &renderer_qml, &RendererQML::redraw);
    // if new videos are added, redraw and reset everything, including the buffer for framerate centering)
    QObject::connect(&file_item_model,                 &FileItemModel::updateFileItemPaths, &renderer_qml, &RendererQML::forced_reshape_redraw);
    // if new videos are added, finish the exporting (closes the csv filehandle)
    QObject::connect(&file_item_model,                 &FileItemModel::updateFileItemPaths, &exporter_qml, &ExporterQML::finishExporting);
    // if new videos are added, update the count so we can export the correct amount of entries for the csv file
    QObject::connect(&file_item_model,                 &FileItemModel::updateFileItemPaths, &exporter_qml, &ExporterQML::updateVideoCount);


    // load application
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    const QStringList args = parser.positionalArguments();
    if (args.length() != 2) { 
        return app.exec();
    };
    QString imagesequence = parser.value("n");
    if (imagesequence.isEmpty()) {
        imagesequence = "default_image_";
    }
    QString source = args.at(0);
    QString destination = args.at(1);


    shared_resolution_model->setActiveValueAt(3);
    shared_export_options_model->setData(QModelIndex(), false, shared_export_options_model->EnableLivePreviewValueRole);
    shared_export_options_model->setData(QModelIndex(), imagesequence, shared_export_options_model->ImagesequencePrefixValueRole);
    shared_export_options_model->setData(QModelIndex(), destination, shared_export_options_model->ExportDirectoryValueRole);
    shared_general_options_model->setData(QModelIndex(), false, shared_general_options_model->EnableTearsValueRole);
    file_item_model.setData(file_item_model.index(0), QString("file://%s").arg(source), file_item_model.QtFilePathRole);
    file_item_model.setData(file_item_model.index(0), source, file_item_model.FilePathRole);
    file_item_model.setData(file_item_model.index(0), true, file_item_model.FileSelectedRole);
    file_item_model.setData(file_item_model.index(0), 60, file_item_model.RecordedFramerateRole);

    file_item_model.setData(file_item_model.index(0), file_item_model.getFileSize(source), file_item_model.SizeMBRole);
    file_item_model.emitFilePaths(QList<QVariant>{source});
    file_item_model.resetModel();
    file_item_model.setRecordedFramerates(QList<QVariant>{source}
            ,videocapturelist_qml.getRecordedFramerates());
    framerate_options_model.updateEnabledRows(QList<QVariant>{source});
    tear_options_model.updateEnabledRows(QList<QVariant>{source});
    shared_export_options_model->setEnabledExportButton(true);
    videocapturelist_qml.openAllPaths(QList<QVariant>{source});
    frame_processing_qml.resetState(videocapturelist_qml.getUnsignedRecordedFramerates());
    exporter_qml.startExporting();
    while (exporter_qml.isExporting()){
        videocapturelist_qml.readNextFrames();
        qInfo("Progress: %.2f", videocapturelist_qml.getShortestVideoProgress().toDouble() * 100);
    }
    return 0;
}
