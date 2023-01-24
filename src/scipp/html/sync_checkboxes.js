/**
 * sync_checkboxes() : Sync the status of 2 different checkboxes by ID.
 * You can use this function as a `onclick` or `onchange` method of an input element.
 * e.g. <input type='checkbox' onclick='sync_checkboxes(this, "target-id")'>
 * Please note that if 2 checkboxes have the exact same ids,
 * the behavior of the sync may not be as expected.
 * @param {HTMLElement} checkbox_elem
 * @param {string} target_id
 */
function sync_checkboxes(checkbox_elem, target_id) {
    var changed_status = checkbox_elem.checked;
    const target_filter = 'input[type="checkbox"][id="'+target_id+'"]'
    var all_checkboxes = document.querySelectorAll(target_filter);
    // querySelectorAll('#{id}') or querySelectorAll(input[id={value}]) doesn't work in some browsers
    // It could be solved by wrapping the value of the attribute with quotation marks
    for (checkbox of all_checkboxes) {
        checkbox.checked = changed_status;
    }
}
