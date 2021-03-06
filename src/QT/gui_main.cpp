/* 
 * Copyright (c) 2012 Brian Kloppenborg
 *
 * If you use this software as part of a scientific publication, please cite as:
 *
 * Kloppenborg, B.; Baron, F. (2012), "SIMTOI: The SImulation and Modeling 
 * Tool for Optical Interferometry" (Version X). 
 * Available from  <https://github.com/bkloppenborg/simtoi>.
 *
 * This file is part of the SImulation and Modeling Tool for Optical 
 * Interferometry (SIMTOI).
 * 
 * SIMTOI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License 
 * as published by the Free Software Foundation version 3.
 * 
 * SIMTOI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public 
 * License along with SIMTOI.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include "gui_main.h"

#include <QWidgetList>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QVariant>
#include <QString>
#include <QMessageBox>
#include <QTreeView>
#include <QStringList>
#include <QFileDialog>
#include <vector>
#include <utility>
#include <fstream>
#include <cmath>

#include "enumerations.h"
#include "CGLWidget.h"
#include "CPosition.h"
#include "CGLShaderList.h"
#include "CMinimizer.h"
#include "CTreeModel.h"
#include "gui_model.h"
#include "gui_general.h"

//Q_DECLARE_METATYPE(ModelTypes);
//Q_DECLARE_METATYPE(CGLShaderList::ShaderTypes);

gui_main::gui_main(QWidget *parent_widget)
    : QMainWindow(parent_widget)
{
	Init();
}

gui_main::~gui_main()
{
	close();
}

void gui_main::Animation_StartStop()
{
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

	CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());

	if(mAnimating)
	{
		widget->EnqueueOperation(GLT_AnimateStop);
		mAnimating = false;
		ui.btnStartStop->setText("Start");
	}
	else
	{
		widget->SetTimestep(ui.spinTimeStep->value());
		widget->EnqueueOperation(GLT_Animate);
		mAnimating = true;
		ui.btnStartStop->setText("Stop");
	}
}

void gui_main::Animation_Reset()
{
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

	CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());

	widget->SetTime(ui.spinTimeStart->value());
	widget->EnqueueOperation(GLT_RenderModels);

}

void gui_main::AddGLArea()
{
    int width = ui.spinModelSize->value();
    int height = ui.spinModelSize->value();
    double scale = ui.spinModelScale->value();

    AddGLArea(width, height, scale);
}

QMdiSubWindow * gui_main::AddGLArea(int model_width, int model_height, double model_scale)
{
	// Create a new subwindow with a title and close button:
    CGLWidget * widget = new CGLWidget(ui.mdiArea, mShaderSourceDir, mKernelSourceDir);
    QMdiSubWindow * sw = ui.mdiArea->addSubWindow(widget, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    sw->setWindowTitle("Model Area");

    // TODO: This is approximately right for my machine, probably not ok on other OSes.
    int frame_width = 8;
    int frame_height = 28;

    sw->setFixedSize(model_width + frame_width, model_height + frame_height);
    //sw->resize(model_width + frame_width, model_height + frame_height);
    sw->show();

    widget->resize(model_width, model_height);
    widget->SetScale(model_scale);
    widget->startRendering();

	// Now connect signals and slots
	connect(widget->GetTreeModel(), SIGNAL(parameterUpdated(void)), this, SLOT(render(void)));

	// If the load data button isn't enabled, turn it on
	ButtonCheck();

	return sw;
}


/// Checks to see which buttons can be enabled/disabled.
void gui_main::ButtonCheck()
{

	ui.btnAddData->setEnabled(false);
	ui.btnModelAdd->setEnabled(false);
	ui.btnModelEdit->setEnabled(false);
	ui.btnModelDelete->setEnabled(false);
	ui.btnRemoveData->setEnabled(false);
	ui.btnStartStop->setEnabled(false);
	ui.btnReset->setEnabled(false);
	ui.btnRunMinimizer->setEnabled(false);
	ui.btnStopMinimizer->setEnabled(false);
	ui.btnSavePhotometry->setEnabled(false);
	ui.btnSaveFITS->setEnabled(false);
	ui.btnSetTime->setEnabled(false);

    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

	ui.btnStartStop->setEnabled(true);
	ui.btnReset->setEnabled(true);
	ui.btnRunMinimizer->setEnabled(true);
	ui.btnStopMinimizer->setEnabled(true);
	ui.btnSavePhotometry->setEnabled(true);
	ui.btnSaveFITS->setEnabled(true);
	ui.btnSetTime->setEnabled(true);

	// Buttons for add/delete data
	ui.btnAddData->setEnabled(true);
	CGLWidget * widget = dynamic_cast<CGLWidget *>(sw->widget());
	if(widget->GetOpenFileModel()->rowCount() > 0)
		ui.btnRemoveData->setEnabled(true);

	// Buttons for add/edit/delete model
	ui.btnModelAdd->setEnabled(true);
	if(widget->GetTreeModel()->rowCount() > 0)
	{
		ui.btnModelEdit->setEnabled(true);
		ui.btnModelDelete->setEnabled(true);
	}
}

void gui_main::AutoClose(QWidget * widget)
{
	QMdiSubWindow * sw = dynamic_cast<QMdiSubWindow *>(widget);
	sw->close();

	// If there are more open subwindows and auto-close is set, close the program.
	if(mAutoClose && ui.mdiArea->subWindowList().size() == 0)
		close();
}

void gui_main::AutoClose(bool auto_close, QMdiSubWindow * sw)
{
	// logically OR the two values:
	mAutoClose = auto_close || mAutoClose;

	if(!mAutoClose)
		return;

	CGLWidget * widget = dynamic_cast<CGLWidget *>(sw->widget());

	connect(widget, SIGNAL(MinimizationFinished(QWidget *)), this, SLOT(AutoClose(QWidget *)));
}

void gui_main::close()
{
	CGLWidget * widget = NULL;
	QMdiSubWindow * sw = NULL;

	QList<QMdiSubWindow *> windows = ui.mdiArea->subWindowList();
    for (int i = int(windows.count()) - 1; i > 0; i--)
    {
    	QMdiSubWindow * sw = windows.at(i);
    	CGLWidget * widget = dynamic_cast<CGLWidget *>(sw->widget());
    	widget->stopRendering();
    	sw->close();
    }

    // Now call the base-class close method.
    QMainWindow::close();
}

void gui_main::closeEvent(QCloseEvent *evt)
{
	close();
    QMainWindow::closeEvent(evt);
}

/// Create a new SIMTOI model area and runs the specified minimization engine on the data.  If close_simtoi is true
/// SIMTOI will automatically exit when all minimization engines have completed execution.
void gui_main::CommandLine(QStringList & data_files, QStringList & model_files, int minimizer, int size, double scale, bool close_simtoi)
{
	QMdiSubWindow * sw = AddGLArea(size, size, scale);
	DataAdd(data_files, sw);
	ModelOpen(model_files, sw);
	MinimizerRun(minimizer, sw);
	AutoClose(close_simtoi, sw);
}

/// Loads OIFITS data into the current selected subwindow
void gui_main::DataAdd()
{
	// Ensure there is a selected widget, if not immediately return.
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    {
		QMessageBox msgBox;
		msgBox.setText("You must have an active model region before you can load data.");
		msgBox.exec();
    	return;
    }

    // Open a dialog, get a list of file that the user selected:
    QFileDialog dialog(this);
    dialog.setDirectory(QString::fromStdString(mOpenDataDir));
    dialog.setNameFilter(tr("Data Files (*.fit *.fits *.oifits)"));
    dialog.setFileMode(QFileDialog::ExistingFiles);

    QStringList filenames;
    QString dir = "";
	if (dialog.exec())
	{
		filenames = dialog.selectedFiles();
		DataAdd(filenames, sw);
	}
}

void gui_main::DataAdd(QStringList & filenames, QMdiSubWindow * sw)
{
	string tmp;
	int dir_size = 0;
	stringstream time_str;
	time_str.precision(4);
	time_str.setf(ios::fixed,ios::floatfield);

    // Get access to the current widget and QStandardItemModel:
    CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());
    QStandardItemModel * model = widget->GetOpenFileModel();
	QList<QStandardItem *> items;

	// Now pull out the data directory (to make file display cleaner)
	if(filenames.size() > 0)
		mOpenDataDir = QFileInfo(filenames[0]).absolutePath().toStdString();

	for(int i = 0; i < filenames.size(); i++)
	{
		items.clear();
		// Tell the widget to load the data file and append a row to its file list:
		tmp = filenames[i].toStdString();
		dir_size = tmp.size() - mOpenDataDir.size();
		widget->LoadData(tmp);

		// Filename
		items.append( new QStandardItem(QString::fromStdString( tmp.substr(mOpenDataDir.size() + 1, dir_size) )));
		// Mean JD
		time_str.str("");
		time_str << widget->GetDataAveJD(widget->GetNDataSets() - 1);
		items.append( new QStandardItem(QString::fromStdString( time_str.str() )));

		model->appendRow(items);
	}

	ButtonCheck();
}


/// Removes the current selected data set.
void gui_main::DataRemove()
{
	// Ensure there is a selected widget, if not immediately return.
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

    // Get access to the current widget, and QStandardItemModel list
    CGLWidget * widget = dynamic_cast<CGLWidget *>(sw->widget());

    // Get the selected indicies:
    QModelIndexList list = ui.treeOpenFiles->selectionModel()->selectedIndexes();
    QStandardItemModel * model = widget->GetOpenFileModel();

    QList<QModelIndex>::iterator it;
    int id = 0;
    for(it = --list.end(); it > list.begin(); it--)
    {
    	id = (*it).row();
    	widget->RemoveData(id);
    	model->removeRow(id, QModelIndex());
    }
}

void gui_main::DeleteGLArea()
{
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

	CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());
    if (widget)
    {
        widget->stopRendering();
        delete widget;
    }

    ButtonCheck();
}

/// Exports the current rendered model to a floating point FITS file
void gui_main::ExportFITS()
{
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

	CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());
	widget->EnqueueOperation(CLT_Init);

    string filename;
    QStringList fileNames;
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("FITS Files (*.fits)"));
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setAcceptMode(QFileDialog::AcceptSave);

	if (dialog.exec())
	{

		fileNames = dialog.selectedFiles();
		filename = fileNames.first().toStdString();

		// Add an extension if it doesn't already exist
		if(filename.substr(filename.size() - 5, 5) != ".fits")
			filename += ".fits";

		// Automatically overwrite files if they already exist.  The dialog should prompt for us.
		filename = "!" + filename;

		widget->SaveImage(filename);
	}
}

/// Animates the display, exporting the photometry at specified intervals
void gui_main::ExportPhotometry()
{
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

	CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());
	widget->EnqueueOperation(CLT_Init);

    double t = ui.spinTimeStart->value();
    double step = ui.spinTimeStep->value();
    double duration = ui.spinTimeDuration->value();
    double stop = t + duration;
    vector< pair<float, float> > output;
    float flux;

    string filename;
    QStringList fileNames;
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("Text Files (*.txt)"));
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setAcceptMode(QFileDialog::AcceptSave);

	if (dialog.exec())
	{
		// Compute the flux
		while(t < stop)
		{
			widget->SetTime(t);
			widget->EnqueueOperation(GLT_RenderModels);
			flux = widget->GetFlux();

			output.push_back( pair<float, float>(t, flux));
			t += step;
		}

		fileNames = dialog.selectedFiles();
		filename = fileNames.first().toStdString();

		if(filename.substr(filename.size() - 4, 4) != ".txt")
			filename += ".txt";
	}

	ofstream outfile;
	outfile.open(filename.c_str());
	outfile.precision(4);
	outfile.setf(ios::fixed,ios::floatfield);
	outfile << "# Time Flux" << endl;
	//outfile << "# Created using the following parameters: " << endl;

	for(int i = 0; i < output.size(); i++)
	{
		outfile << output[i].first << " " << -2.5*log10(output[i].second) << endl;
	}

	outfile.close();

}

/// Runs initialization routines for the main UI.
void gui_main::Init(void)
{
	// Init the UI
	ui.setupUi(this);

	// Setup signals and slots

	// Model area
	connect(ui.btnModelArea, SIGNAL(clicked(void)), this, SLOT(AddGLArea(void)));
	connect(ui.mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(subwindowSelected(QMdiSubWindow*)));

	// Animiation buttons
	connect(ui.btnStartStop, SIGNAL(clicked(void)), this, SLOT(Animation_StartStop(void)));
	connect(ui.btnReset, SIGNAL(clicked(void)), this, SLOT(Animation_Reset(void)));

	// Minimizer:
	connect(ui.btnRunMinimizer, SIGNAL(clicked(void)), this, SLOT(MinimizerRun(void)));
	connect(ui.btnStopMinimizer, SIGNAL(clicked(void)), this, SLOT(MinimizerStop(void)));

	// Add/delete data
	connect(ui.btnAddData, SIGNAL(clicked(void)), this, SLOT(DataAdd(void)));
	connect(ui.btnRemoveData, SIGNAL(clicked(void)), this, SLOT(DataRemove(void)));

	// Add/delete models
	connect(ui.btnModelAdd, SIGNAL(clicked(void)), this, SLOT(ModelAdd(void)));
	connect(ui.btnModelEdit, SIGNAL(clicked(void)), this, SLOT(ModelEdit(void)));
	connect(ui.btnModelDelete, SIGNAL(clicked(void)), this, SLOT(ModelDelete(void)));

	// File menu:
	connect(ui.actionSave, SIGNAL(triggered(void)), this, SLOT(ModelSave(void)));
	connect(ui.actionOpen, SIGNAL(triggered(void)), this, SLOT(ModelOpen(void)));

	// Flux simulation
	connect(ui.btnSavePhotometry, SIGNAL(clicked(void)), this, SLOT(ExportPhotometry(void)));

	// FITS exporting:
	connect(ui.btnSaveFITS, SIGNAL(clicked(void)), this, SLOT(ExportFITS(void)));

	// Set time button
	connect(ui.btnSetTime, SIGNAL(clicked(void)), this, SLOT(SetTime(void)));

	// Now init variables:

	mAnimating = false;
	mAutoClose = false;

	// Get the application path,
	string app_path = QCoreApplication::applicationDirPath().toStdString();
	mShaderSourceDir = app_path + "/shaders/";
	mKernelSourceDir = app_path + "/kernels/";

	mOpenDataDir = "./";
	mOpenModelDir = "./";

	mDefaultSaveDir = "/tmp/model";
	mNumMinimizations = 0;

	// Setup the combo boxes.
	SetupComboBoxes();

	// Setup the text boxes
	ui.textSavePath->setText(mDefaultSaveDir.c_str());
}

void gui_main::MinimizerRun()
{
	int minimizer = ui.cboMinimizers->itemData(ui.cboMinimizers->currentIndex()).toInt();

    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

    MinimizerRun(minimizer, sw);
}

void gui_main::MinimizerRun(int minimizer_id, QMdiSubWindow * sw)
{
	CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());
    CMinimizer::MinimizerTypes minimizer;

    if(widget->GetNData() == 0)
    {
		QMessageBox msgBox;
		msgBox.setText("You must load data before running a minimizer.");
		msgBox.exec();
    	return;
    }

	widget->EnqueueOperation(CLT_Init);

    // Now determine which minimizer is selected.  The else case could be generated
	// from a command-line execution of SIMTOI.
	if(minimizer_id > CMinimizer::NONE && minimizer_id < CMinimizer::LAST_VALUE)
	{
		minimizer = CMinimizer::MinimizerTypes(minimizer_id);
	    widget->LoadMinimizer(minimizer);

	    // set the save file base name
	    string ui_savefile = ui.textSavePath->text().toStdString();
	    if(ui_savefile == mDefaultSaveDir)
	    {
	    	std::stringstream savepath;
	    	savepath << ui_savefile << mNumMinimizations;
	    	widget->SetSaveFileBasename(savepath.str());
	    }
	    else
	    {
	    	widget->SetSaveFileBasename(ui_savefile);
	    }

	    widget->RunMinimizer();
	    mNumMinimizations += 1;
	}
	else
    {
		QMessageBox msgBox;
		msgBox.setText("Unknown minimizer.");
		msgBox.exec();
    	return;
    }
}

/// Stops the minimizer
void gui_main::MinimizerStop()
{
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

	CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());
    CMinimizer::MinimizerTypes minimizer;

	widget->StopMinimizer();
}

void gui_main::ModelAdd(void)
{
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

	CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());
    int id = 0;
    int n_features;

    gui_model tmp;
    tmp.SetModelTypes(CModelList::GetTypes());
    tmp.SetShaderTypes(widget->GetShaderTypes());
    tmp.SetPositionTypes(CPosition::GetTypes());
    tmp.show();

    if(tmp.exec())
    {
		// Now setup the model, position type, and shader.
		widget->AddModel(tmp.GetModelType());
		id = widget->GetNModels() - 1;
		widget->SetPositionType(id, tmp.GetPositionType());
		widget->SetShader(id, tmp.GetShaderType());

    }

    // Now render the models:
    widget->EnqueueOperation(GLT_RenderModels);
}

/// Deletes the selected model from the model list
void gui_main::ModelDelete()
{
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

	CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());

	// TODO: Finish this function

}

/// Opens up an editing dialog for the currently selected model.
void gui_main::ModelEdit()
{
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

    // see if an item is selected in the tree view, get it's ID
    // then pull out the model's informatoin
    // TODO: finish this.

//	CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());
//    int id = 0;
//    int n_features;
//
//    // setup the gui:
//    gui_model tmp;
//    tmp.SetModelTypes(CModelList::GetTypes());
//    tmp.SetShaderTypes(widget->GetShaderTypes());
//    tmp.SetPositionTypes(CPosition::GetTypes());
//
//    // set the specific information from the current selected model
//
//    tmp.show();
//
//    if(tmp.exec())
//    {
//		// Now setup the model, position type, and shader.
//		widget->AddModel(tmp.GetModelType());
//		id = widget->GetNModels() - 1;
//		widget->SetPositionType(id, tmp.GetPositionType());
//		widget->SetShader(id, tmp.GetShaderType());
//
//    }
//
//    // Now render the models:
//    widget->EnqueueOperation(GLT_RenderModels);
}

void gui_main::ModelOpen()
{
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDirectory(QString::fromStdString(mOpenModelDir));
    dialog.setNameFilter(tr("SIMTOI Save Files (*.json)"));
    dialog.setViewMode(QFileDialog::Detail);
    QStringList fileNames;
	if (dialog.exec())
	{
		fileNames = dialog.selectedFiles();

		ModelOpen(fileNames, sw);
	}
}

void gui_main::ModelOpen(QStringList & fileNames, QMdiSubWindow * sw)
{
	CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());
	widget->Open(fileNames.first().toStdString());

	// Store the directory name for the next file open dialog.
	mOpenModelDir = QFileInfo(fileNames[0]).absolutePath().toStdString();
}

void gui_main::ModelSave()
{
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

    string filename;
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("SIMTOI Save Files (*.json)"));
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    QStringList fileNames;
	if (dialog.exec())
	{
		fileNames = dialog.selectedFiles();
		filename = fileNames.first().toStdString();
		string tmp =filename.substr(filename.size() - 5, 5);

		if(filename.substr(filename.size() - 5, 5) != ".json")
			filename += ".json";

		CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());
		widget->Save(filename);
	}
}

void gui_main::render()
{
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

	CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());
    if(widget)
    {
    	widget->EnqueueOperation(GLT_RenderModels);
    }
}

void gui_main::SetupComboBoxes()
{
	gui_general::SetupComboOptions(ui.cboMinimizers, CMinimizer::GetTypes());
}

void gui_main::SetSavePath(void)
{
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

	CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());

    // set the save file base name
    string ui_savefile = ui.textSavePath->text().toStdString();
    if(ui_savefile == mDefaultSaveDir)
    {
    	std::stringstream savepath;
    	savepath << ui_savefile << mNumMinimizations;
    	widget->SetSaveFileBasename(savepath.str());
    }
    else
    {
    	widget->SetSaveFileBasename(ui_savefile);
    }

    widget->RunMinimizer();
    mNumMinimizations += 1;
}

/// Sets the time from the current selected datafile.
void gui_main::SetTime(void)
{
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

    // Get access to the current widget, and QStandardItemModel list
    CGLWidget * widget = dynamic_cast<CGLWidget *>(sw->widget());

    // Get the selected indicies:
    QModelIndexList list = ui.treeOpenFiles->selectionModel()->selectedIndexes();
    QStandardItemModel * model = widget->GetOpenFileModel();

    QList<QModelIndex>::iterator it = list.begin();
    int id = (*it).row();
    double t = widget->GetDataAveJD(id);
    widget->SetTime(t);
    widget->EnqueueOperation(GLT_RenderModels);
}

/// Called when a subwindow is selected.
void gui_main::subwindowSelected(QMdiSubWindow * mdi_subwindow)
{
	ButtonCheck();
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

	CGLWidget *widget = dynamic_cast<CGLWidget*>(sw->widget());

	// Configure the open file widget:
	ui.treeOpenFiles->setHeaderHidden(false);
    ui.treeOpenFiles->setModel(widget->GetOpenFileModel());
	ui.treeOpenFiles->header()->setResizeMode(QHeaderView::ResizeToContents);

	// Configure the model tree
    CTreeModel * model = widget->GetTreeModel();
	ui.treeModels->setHeaderHidden(false);
	ui.treeModels->setModel(widget->GetTreeModel());
	ui.treeModels->header()->setResizeMode(QHeaderView::ResizeToContents);

	// Configure the Minimizer box
	ui.textSavePath->setText(widget->GetSaveFileBasename().c_str());
}

