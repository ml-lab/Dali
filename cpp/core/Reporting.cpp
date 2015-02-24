#include "Reporting.h"

DEFINE_string(save, "", "Where to save model to ?");
DEFINE_int32(save_frequency_s, 60, "How often to save model (in seconds) ?");



void Throttled::maybe_run(Clock::duration time_between_actions, std::function<void()> f) {
    std::lock_guard<decltype(lock)> lg(lock);
    if (Clock::now() - last_report >= time_between_actions) {
        f();
        last_report = Clock::now();
    }
}

template<typename T>
ReportProgress<T>::ReportProgress(string name,
               const double& total_work,
               Clock::duration report_frequency) :
        name(name),
        total_work(total_work),
        report_frequency(report_frequency) {
}

template<typename T>
void ReportProgress<T>::tick(const double& completed_work, T work) {
    t.maybe_run(report_frequency, [this, &completed_work, &work]() {
        int active_bars = RESOLUTION * completed_work/total_work;
        std::stringstream ss;
        ss << "\r" << name << " [";
        for (int i = 0; i < RESOLUTION; ++i) {
            if (i < active_bars) {
                ss << "█";
            } else {
                ss << " ";
            }
        }
        ss << "] " << std::fixed
                   << std::setprecision( 3 ) // use 3 decimals
                   << std::setfill( ' ' ) <<  100.0*completed_work/total_work << "%";
        ss << " " << work;
        std::cout << ss.str();
        std::cout.flush();
    });
}

template<typename T>
void ReportProgress<T>::done() {
    std::cout << "\r" << name << " done                          " << std::endl;
}

template<typename T>
void maybe_save_model(const T& model,
                      const string& base_path,
                      const string& label) {
    if (base_path.empty() && FLAGS_save.empty()) return;
    if (FLAGS_save != "") {
        model_save_throttled.maybe_run(seconds(FLAGS_save_frequency_s),
                [&model,&base_path,&label]() {
            std::stringstream filename;
            if(!base_path.empty()) {
                filename << base_path;
            } else if (!FLAGS_save.empty()) {
                filename << FLAGS_save;
            }
            if(!label.empty())
                filename << "_" << label;

            filename << "_" << model_snapshot_no;
            model_snapshot_no += 1;

            std::cout << "Saving model to \""
                      << filename.str() << "/\"" << std::endl;

            model.save(filename.str());
        });
    }
}

template class ReportProgress<double>;

template void maybe_save_model<StackedModel<float> >(const StackedModel<float>&, const string& base_path, const string& label);
template void maybe_save_model<StackedModel<double> >(const StackedModel<double>&, const string& base_path, const string& label);

template void maybe_save_model<StackedShortcutModel<float> >(const StackedShortcutModel<float>&, const string& base_path, const string& label);
template void maybe_save_model<StackedShortcutModel<double> >(const StackedShortcutModel<double>&, const string& base_path, const string& label);

template void maybe_save_model<StackedGatedModel<float> >(const StackedGatedModel<float>&, const string& base_path, const string& label);
template void maybe_save_model<StackedGatedModel<double> >(const StackedGatedModel<double>&, const string& base_path, const string& label);
