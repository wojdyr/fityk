// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "logic.h"
#include <stdio.h>
#include <fstream>
#include "data.h"
#include "sum.h"
#include "ui.h"
#include "fit.h"
#include "guess.h"

using namespace std;

ApplicationLogic *AL;

DataWithSum::DataWithSum(VariableManager *mgr, Data* data_)
    : data(data_ ? data_ : new Data), sum(new Sum(mgr, data.get()))  
{}

void ApplicationLogic::activate_ds(int d)
{
    if (d < 0 || d >= size(dsds))
        throw ExecuteError("there is no such dataset: @" + S(d));
    active_ds = d;
    view.set_dataset(get_ds(active_ds));
}

int ApplicationLogic::append_ds(Data *data)
{
    DataWithSum* ds = new DataWithSum(this, data);
    dsds.push_back(ds); 
    return dsds.size()-1; 
}

void ApplicationLogic::remove_ds(int d)
{
    if (d < 0 || d >= size(dsds))
        throw ExecuteError("there is no such dataset: @" + S(d));
    delete dsds[d];
    dsds.erase(dsds.begin() + d);
    if (dsds.empty())
        append_ds();
    if (active_ds == d)
        activate_ds( d==size(dsds) ? d-1 : d );
}


//TODO ? this func should not be neccessary
void ApplicationLogic::reset_all (bool finish) 
{
    dsds.clear();
    parameters.clear();
    if (finish)
        return;
    view = View(0, 180, 0, 1e3);
    append_ds();
    activate_ds(0);
}


void ApplicationLogic::dump_all_as_script(string const &filename)
{
    ofstream os(filename.c_str(), ios::out);
    if (!os) {
        warn ("Can't open file: " + filename);
        return;
    }
    os << fityk_version_line << endl;
    os << "####### Dump time: " << time_now() << endl << endl;
    //TODO dump_all_as_script
#if 0
    params->export_as_script(os);
    os << endl;

    for (int i = 0; i != size(cores); i++) {
        if (cores.size() > 1) 
            os << endl << "### core of plot #" << i << endl;
        if (i != 0)
            os << "d.activate *::" << endl; 
        cores[i]->export_as_script(os);
        os << endl;
    }
    if (active_core != size(cores) - 1)
        os << "d.activate " << active_core << ":: # set active" << endl; 
    os << "plot " << my_core->view.str() << endl;



void PlotCore::export_as_script(std::ostream& os) const
{
    for (int i = 0; i != size(datasets); i++) {
        os << "#dataset " << i << endl;
        if (i != 0)
            os << "d.activate ::*" << endl;
        datasets[i]->export_as_script(os);
        os << endl;
    }
    if (active_data != size(datasets) - 1)
        os << "d.activate ::" << active_data << " # set active" << endl; 
    sum->export_as_script(os);
    os << endl;
}
#endif
    os << endl;
    os << endl << "####### End of dump " << endl; 
}


DataWithSum* ApplicationLogic::get_ds(int n)
{
    if (n == -1) {
        if (get_ds_count() == 1)
            return dsds[0];
        else
            throw ExecuteError("Dataset must be specified.");
    }
    if (n < 0 || n >= get_ds_count())
        throw ExecuteError("There is no dataset @" + S(n));
    return dsds[n];
}

//==================================================================


const fp View::relative_x_margin = 1./20.;
const fp View::relative_y_margin = 1./20.;

string View::str() const
{ 
    return "[" + (left!=right ? S(left) + ":" + S(right) : string(" "))
        + "] [" + (bottom!=top ? S(bottom) + ":" + S(top) 
                                           : string (" ")) + "]";
}

void View::fit(int flag)
{
    if (flag&fit_left || flag&fit_right) {
        fp x_min, x_max;
        get_x_range(x_min, x_max);
        if (x_min == x_max) {
            x_min -= 0.1; 
            x_max += 0.1;
        }
        fp x_margin = (x_max - x_min) * relative_x_margin;
        if (flag&fit_left)
            left = x_min - x_margin;
        if (flag&fit_right)
            right = x_max + x_margin;
    }

    if (flag&fit_top || flag&fit_bottom) {
        fp y_min, y_max;
        get_y_range(y_min, y_max);
        if (y_min == y_max) {
            y_min -= 0.1; 
            y_max += 0.1;
        }
        fp y_margin = (y_max - y_min) * relative_y_margin;
        if (flag&fit_bottom)
            bottom = y_min - y_margin;
        if (flag&fit_top)
            top = y_max + y_margin;
    }

}

void View::get_x_range(fp &x_min, fp &x_max)
{
        if (datas.empty()) 
            throw ExecuteError("Can't find x-y axes ranges for plot");
        x_min = datas.front()->get_x_min();
        x_max = datas.front()->get_x_max();
        for (vector<Data*>::const_iterator i = datas.begin()+1; 
                i != datas.end(); ++i) {
            x_min = min(x_min, (*i)->get_x_min());
            x_max = max(x_max, (*i)->get_x_max());
        }
}

void View::get_y_range(fp &y_min, fp &y_max)
{
    y_min = y_max = 0;
    bool min_max_set = false;
    for (vector<Data*>::const_iterator i = datas.begin(); i != datas.end();
            ++i) {
        vector<Point>::const_iterator f = (*i)->get_point_at(left);
        vector<Point>::const_iterator l = (*i)->get_point_at(right);
        //first we are searching for minimal and max. y in active points
        for (vector<Point>::const_iterator i = f; i < l; i++) {
            if (i->is_active) {
                min_max_set = true;
                if (i->y > y_max) 
                    y_max = i->y;
                if (i->y < y_min) 
                    y_min = i->y;
            }
        }
    }

    if (!min_max_set || y_min == y_max) { //none or 1 active point, so now we  
                                   // search for min. and max. y in all points 
        for (vector<Data*>::const_iterator i = datas.begin(); i != datas.end();
                ++i) {
            vector<Point>::const_iterator f = (*i)->get_point_at(left);
            vector<Point>::const_iterator l = (*i)->get_point_at(right);
            for (vector<Point>::const_iterator i = f; i < l; i++) { 
                min_max_set = true;
                if (i->y > y_max) 
                    y_max = i->y;
                if (i->y < y_min) 
                    y_min = i->y;
            }
        }
    }

    for (vector<Sum*>::const_iterator i = sums.begin(); i != sums.end(); ++i) {
        Sum *sum = *i;
        // estimated sum maximum
        fp sum_y_max = sum->approx_max(left, right);
        if (sum_y_max > y_max) {
            y_max = sum_y_max;
        }
    }
}

void View::parse_and_set(std::vector<std::string> const& lrbt) 
{
    assert(lrbt.size() == 4);
    string const &left = lrbt[0];
    string const &right = lrbt[1];
    string const &bottom = lrbt[2];
    string const &top = lrbt[3];
    fp l=0., r=0., b=0., t=0.;
    int flag = 0;
    if (left.empty())
        flag |= fit_left;
    else if (left != ".") {
        flag |= change_left;
        l = strtod(left.c_str(), 0);
    }
    if (right.empty())
        flag |= fit_right;
    else if (right != ".") {
        flag |= change_right;
        r = strtod(right.c_str(), 0);
    }
    if (bottom.empty())
        flag |= fit_bottom;
    else if (bottom != ".") {
        flag |= change_bottom;
        b = strtod(bottom.c_str(), 0);
    }
    if (top.empty())
        flag |= fit_top;
    else if (top != ".") {
        flag |= change_top;
        t = strtod(top.c_str(), 0);
    }
    set(l, r, b, t, flag);
    fit(flag);
}

void View::set_dataset(DataWithSum const* ds) 
{ 
    set_items(vector1(ds->get_data()), vector1(ds->get_sum())); 
}

