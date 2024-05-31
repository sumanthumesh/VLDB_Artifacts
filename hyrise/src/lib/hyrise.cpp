#include "hyrise.hpp"

namespace hyrise
{

Hyrise::Hyrise()
{
    // The default_memory_resource must be initialized before Hyrise's members
    // so that it is destructed after them and remains accessible during their
    // deconstruction. For example, when the StorageManager is destructed, it
    // causes its stored tables to be deconstructed, too. As these might call
    // deallocate on the default_memory_resource, it is important that the
    // resource has not been destructed before. As objects are destructed in the
    // reverse order of their construction, explicitly initializing the resource
    // first means that it is destructed last.
    boost::container::pmr::get_default_resource();

    storage_manager = StorageManager{};
    plugin_manager = PluginManager{};
    transaction_manager = TransactionManager{};
    meta_table_manager = MetaTableManager{};
    settings_manager = SettingsManager{};
    log_manager = LogManager{};
    topology = Topology{};
    _scheduler = std::make_shared<ImmediateExecutionScheduler>();
}

void Hyrise::reset()
{
    Hyrise::get().scheduler()->finish();
    get() = Hyrise{};
}

const std::shared_ptr<AbstractScheduler> &Hyrise::scheduler() const
{
    return _scheduler;
}

bool Hyrise::is_multi_threaded() const
{
    return std::dynamic_pointer_cast<ImmediateExecutionScheduler>(_scheduler) ==
           nullptr;
}

void Hyrise::set_scheduler(
    const std::shared_ptr<AbstractScheduler> &new_scheduler)
{
    _scheduler->finish();
    _scheduler = new_scheduler;
    _scheduler->begin();
}

void Hyrise::set_pintool(bool value)
{
    //Set the pintool enabled value to true or false
    _pintool_enabled = value;
}

bool Hyrise::is_pin_enabled()
{
    return _pintool_enabled;
}

void Hyrise::set_iotop(bool value)
{
    //Set the pintool enabled value to true or false
    _iotop_enabled = value;
}

bool Hyrise::is_iotop_enabled()
{
    return _iotop_enabled;
}

void Hyrise::set_iostat(bool value)
{
    //Set the pintool enabled value to true or false
    _iostat_enabled = value;
}

bool Hyrise::is_iostat_enabled()
{
    return _iostat_enabled;
}

void Hyrise::set_vtune(bool value)
{
    //Set the pintool enabled value to true or false
    _vtune_enabled = value;
}

bool Hyrise::is_vtune_enabled()
{
    return _vtune_enabled;
}

void Hyrise::set_coreutil(bool value)
{
    //Set the pintool enabled value to true or false
    _coreutil_enabled = value;
}

bool Hyrise::is_coreutil_enabled()
{
    return _coreutil_enabled;
}
int Hyrise::get_query_count()
{
    return _query_count;
}

void Hyrise::incr_query_count()
{
    _query_count++;
}
} // namespace hyrise
