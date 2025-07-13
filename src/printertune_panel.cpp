#include "printertune_panel.h"
#include "state.h"
#include "spdlog/spdlog.h"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

LV_IMG_DECLARE(bedmesh_img);
LV_IMG_DECLARE(fine_tune_img);
LV_IMG_DECLARE(inputshaper_img);
LV_IMG_DECLARE(limit_img);
LV_IMG_DECLARE(motor_img);
LV_IMG_DECLARE(chart_img);

#ifndef ZBOLT
LV_IMG_DECLARE(belts_calibration_img);
LV_IMG_DECLARE(power_devices_img);
#else
LV_IMG_DECLARE(print);
#endif

PrinterTunePanel::PrinterTunePanel(KWebSocketClient &c, std::mutex &l, lv_obj_t *parent, FineTunePanel &finetune)
  : cont(lv_obj_create(parent))
  , bedmesh_panel(c, l)
  , finetune_panel(finetune)
  , limits_panel(c, l)
  , inputshaper_panel(c, l)
  , belts_calibration_panel(c, l)
  , tmc_tune_panel(c)
  , tmc_status_panel(c, l)
  , power_panel(c, l)
  , bedmesh_btn(cont, &bedmesh_img, "Bed Mesh", &PrinterTunePanel::_handle_callback, this)
  , finetune_btn(cont, &fine_tune_img, "Fine Tune", &PrinterTunePanel::_handle_callback, this)
  , inputshaper_btn(cont, &inputshaper_img, "Input Shaper", &PrinterTunePanel::_handle_callback, this)
#ifndef ZBOLT
  , belts_calibration_btn(cont, &belts_calibration_img, "Belts/Shake", &PrinterTunePanel::_handle_callback, this)
#else
  , belts_calibration_btn(cont, &inputshaper_img, "Belts/Shake", &PrinterTunePanel::_handle_callback, this)
#endif
  , limits_btn(cont, &limit_img, "Limits", &PrinterTunePanel::_handle_callback, this)
  , tmc_tune_btn(cont, &motor_img, "TMC Autotune", &PrinterTunePanel::_handle_callback, this)
  , tmc_status_btn(cont, &chart_img, "TMC Metrics", &PrinterTunePanel::_handle_callback, this)
#ifndef ZBOLT
  , power_devices_btn(cont, &power_devices_img, "Power Devices", &PrinterTunePanel::_handle_callback, this)
#else
  , power_devices_btn(cont, &print, "Power Devices", &PrinterTunePanel::_handle_callback, this)
#endif
{
  lv_obj_move_background(cont);

  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));

  tmc_tune_btn.disable();

  static lv_coord_t grid_main_row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(5), LV_GRID_FR(5), LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST};
  static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
      LV_GRID_TEMPLATE_LAST};

  lv_obj_set_grid_dsc_array(cont, grid_main_col_dsc, grid_main_row_dsc);

  // row 1
  lv_obj_set_grid_cell(bedmesh_btn.get_container(), LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_set_grid_cell(finetune_btn.get_container(), LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_set_grid_cell(inputshaper_btn.get_container(), LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_set_grid_cell(belts_calibration_btn.get_container(), LV_GRID_ALIGN_STRETCH, 3, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

  // row 2
  lv_obj_set_grid_cell(limits_btn.get_container(), LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
  lv_obj_set_grid_cell(tmc_tune_btn.get_container(), LV_GRID_ALIGN_STRETCH, 3, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
  lv_obj_set_grid_cell(tmc_status_btn.get_container(), LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
  lv_obj_set_grid_cell(power_devices_btn.get_container(), LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
  // lv_obj_set_grid_cell(restart_firmware_btn.get_container(), LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_START, 2, 1);
}

PrinterTunePanel::~PrinterTunePanel() {
  if (cont != NULL) {
    lv_obj_del(cont);
    cont = NULL;
  }
}

lv_obj_t *PrinterTunePanel::get_container() {
  return cont;
}

BedMeshPanel &PrinterTunePanel::get_bedmesh_panel() {
  return bedmesh_panel;
}

PowerPanel &PrinterTunePanel::get_power_panel() {
  return power_panel;
}

void PrinterTunePanel::init(json &j) {
  limits_panel.init(j);

  tmc_status_panel.init(j);

  // TODO: handle remote guppy instance
  State *s = State::get_instance();
  auto kp = s->get_data("/printer_info/klipper_path"_json_pointer);
  if (!kp.is_null()) {
    auto p = fs::path(kp.template get<std::string>()) / "klippy/extras/motor_database.cfg";
    if (fs::exists(p)) {
      tmc_tune_btn.enable();
      tmc_tune_panel.init(j, p);
    } else {
      tmc_tune_btn.disable();
    }
  }
}

void PrinterTunePanel::handle_callback(lv_event_t *event) {
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    lv_obj_t *btn = lv_event_get_current_target(event);

    if (btn == finetune_btn.get_container()) {
      spdlog::trace("tune finetune pressed");
      finetune_panel.foreground();
    } else if (btn == bedmesh_btn.get_container()) {
      spdlog::trace("tune bedmesh pressed");
      bedmesh_panel.foreground();
    } else if (btn == inputshaper_btn.get_container()) {
      spdlog::trace("tune inputshaper pressed");
      inputshaper_panel.foreground();
    } else if (btn == belts_calibration_btn.get_container()) {
      spdlog::trace("tune belts pressed");
      belts_calibration_panel.foreground();
    } else if (btn == limits_btn.get_container()) {
      spdlog::trace("limits pressed");
      limits_panel.foreground();
    } else if (btn == tmc_tune_btn.get_container()) {
      spdlog::trace("tmc auto tune pressed");
      tmc_tune_panel.foreground();
    } else if (btn == tmc_status_btn.get_container()) {
      spdlog::trace("tmc metrics pressed");
      tmc_status_panel.foreground();
    } else if (btn == power_devices_btn.get_container()) {
      spdlog::trace("power devices pressed");
      power_panel.foreground();
    }
  }
}
