// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// DraggedFunc - used for dragging function in MainPlot

#ifndef FITYK_WX_FDRAG_H_
#define FITYK_WX_FDRAG_H_

#include "fityk/common.h"

namespace fityk { class Function; class ModelManager; }

class DraggedFuncObserver
{
public:
    virtual ~DraggedFuncObserver() {}
    virtual void change_parameter_value(int idx, double value) = 0;
};

class DraggedFunc
{
public:
    enum DragType
    {
        no_drag,
        relative_value, //eg. for area
        absolute_value,  //eg. for width
        absolute_pixels
    };

    struct Drag
    {
        DragType how;
        int parameter_idx;
        std::string parameter_name;
        std::string variable_name; /// name of variable that are to be changed
        double value; /// current value of parameter
        double ini_value; /// initial value of parameter
        double multiplier; /// increases or decreases changing rate
        double ini_x;

        Drag() : how(no_drag) {}
    };

    DraggedFunc(const fityk::ModelManager& mgr)
        : mgr_(mgr), has_changed_(false) {}
    void start(const fityk::Function* p, int X, int Y, double x, double y,
               DraggedFuncObserver* callback);
    void move(bool shift, int X, int Y, double x, double y);
    void stop();
    const std::string& status() const { return status_; }
    bool has_changed() { return has_changed_; }
    std::string get_cmd() const;

private:
    const fityk::ModelManager& mgr_;
    DraggedFuncObserver* callback_;
    Drag drag_x_; ///for horizontal dragging (x axis)
    Drag drag_y_; /// y axis
    Drag drag_shift_x_; ///x with [shift]
    Drag drag_shift_y_; ///y with [shift]
    double px_, py_;
    int pX_, pY_;
    std::string status_;
    bool has_changed_;

    void change_value(Drag *drag, double x, double dx, int dX);
};

#endif

