#include "stdafx.h"
#include <cstdlib>

zend_class_entry *event_entry;
zend_class_entry *callback_entry;

PHP_FUNCTION(Constructor)
{	
	zval *objvar = getThis();	
    zval *arr;
	MAKE_STD_ZVAL(arr);
	array_init(arr);
	add_property_zval(objvar, "list", arr);
}

PHP_FUNCTION(_Subscriptions)
{
	zval **tmp;
	HashTable *prop = Z_OBJPROP_P(getThis());

	if (zend_hash_find(prop, "list", sizeof("list"), (void**)&tmp) == SUCCESS && Z_TYPE_PP(tmp) != IS_NULL) 
	{
		RETURN_LONG(zend_hash_num_elements(Z_ARRVAL_P(*tmp)));
	}
}

PHP_FUNCTION(_Subscribe)
{
	zval *list, **tmp, *callback, *object;
	zval *objvar = getThis();
	char *method;
	int method_len;
	HashTable *prop = Z_OBJPROP_P(getThis());

	if (zend_hash_find(prop, "list", sizeof("list"), (void**)&tmp) == SUCCESS && Z_TYPE_PP(tmp) != IS_NULL) 
	{
		list = *tmp;
	}
	
	if (ZEND_NUM_ARGS() == 1)
	{
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &method, &method_len) == FAILURE)
		{
			return;
		}
		else
		{
			add_next_index_string(list, method, 1);
		}
	}
	else if (ZEND_NUM_ARGS() == 2)
	{
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zs", &object, &method, &method_len) == FAILURE)
		{
			return;
		}
		else
		{
			MAKE_STD_ZVAL(callback);
			object_init_ex(callback, callback_entry);
			add_property_zval(callback,"context", object);
			add_property_string(callback,"method", method, 1);
			add_next_index_zval(list, callback);	
		}
	}
	else
	{
		zend_error(E_WARNING, "Incorrect parameters added");
		return;			
	}
}

PHP_FUNCTION(_Unsubscribe)
{
	zval **tmp, **attr, *object;
	char *method;
	int method_len;
	HashTable* prop = Z_OBJPROP_P(getThis());
	HashPosition pos;	
	
	if (ZEND_NUM_ARGS() == 1)
	{
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &method, &method_len) == FAILURE)
		{
			return;
		}
		else
		{
			MAKE_STD_ZVAL(object);			
			ZVAL_NULL(object);
		}
	}
	else if (ZEND_NUM_ARGS() == 2)
	{
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zs", &object, &method, &method_len) == FAILURE)
		{
			return;
		}
	}
	else
	{
		zend_error(E_WARNING, "Incorrect parameters added");
		return;			
	}

	if (zend_hash_find(prop, "list", sizeof("list"), (void**)&tmp) == SUCCESS && Z_TYPE_PP(tmp) != IS_NULL) 
	{
 		zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(*tmp), &pos);
		while (zend_hash_get_current_data_ex(Z_ARRVAL_P(*tmp), (void**)&attr, &pos) == SUCCESS)
		{
			zval *method2, *object2; 
			char *method2str;

			if (Z_TYPE_P(*attr) == IS_OBJECT)
			{
				method2 = zend_read_property(callback_entry, *attr, "method", sizeof("method")-1, 1 TSRMLS_CC);
				object2 = zend_read_property(callback_entry, *attr, "context", sizeof("context")-1, 1 TSRMLS_CC);			
				method2str = Z_STRVAL_P(method2);
				
				if ((strcmp(method2str,method) == 0) && (object2->type == object->type))
				{
					zend_hash_index_del(Z_ARRVAL_P(*tmp), pos->h);
				}
			}
			else
			{
				method2str = Z_STRVAL_P(*attr);
				if ((strcmp(method2str,method) == 0) && (ZVAL_IS_NULL(object)))
				{
					zend_hash_index_del(Z_ARRVAL_P(*tmp), pos->h);
				}
			}
			zend_hash_move_forward_ex(Z_ARRVAL_P(*tmp), &pos);
		}
	}	
}

PHP_FUNCTION(_Raise)
{
	zval *object, *method, **tmp, **attr, ***args;
	HashTable *prop;
	HashPosition pos;
	prop = Z_OBJPROP_P(getThis());

	args = (zval ***)emalloc(ZEND_NUM_ARGS() * sizeof(zval **));
	zend_get_parameters_array_ex(ZEND_NUM_ARGS(), args);

	if (zend_hash_find(prop, "list", sizeof("list"), (void**)&tmp) == SUCCESS && Z_TYPE_PP(tmp) != IS_NULL) 
	{
 		zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(*tmp), &pos);
		while (zend_hash_get_current_data_ex(Z_ARRVAL_P(*tmp), (void**)&attr, &pos) == SUCCESS)
		{
			if (Z_TYPE_P(*attr) == IS_OBJECT)
			{
				method = zend_read_property(callback_entry, *attr, "method", sizeof("method")-1, 1 TSRMLS_CC);
				object = zend_read_property(callback_entry, *attr, "context", sizeof("context")-1, 1 TSRMLS_CC);

				if (call_user_function_ex(NULL, &object, method, &return_value, ZEND_NUM_ARGS(), args, 0, NULL TSRMLS_CC) == FAILURE)
				{
					zend_error(E_WARNING, "%s does not exit", Z_STRVAL_P(method));
				}
			}
			else
			{				
				if (call_user_function_ex(CG(function_table), NULL, *attr, &return_value, ZEND_NUM_ARGS(), args, 0, NULL TSRMLS_CC) == FAILURE)
				{					
					zend_error(E_WARNING, "%s does not exit", Z_STRVAL_P(method));				
				}
			}
			zend_hash_move_forward_ex(Z_ARRVAL_P(*tmp), &pos);
		}
	}
}

static function_entry event_functions[] = {
	PHP_NAMED_FE(__construct, PHP_FN(Constructor), NULL)
	PHP_NAMED_FE(Subscriptions, PHP_FN(_Subscriptions), NULL)
	PHP_NAMED_FE(Raise, PHP_FN(_Raise), NULL)
	PHP_NAMED_FE(Subscribe, PHP_FN(_Subscribe), NULL)
	PHP_NAMED_FE(Unsubscribe, PHP_FN(_Unsubscribe), NULL)
    { NULL, NULL, NULL }
};

PHP_MINIT_FUNCTION(event)
{
	zend_class_entry ce, ce2;
    INIT_CLASS_ENTRY(ce, "Event", event_functions);	
    event_entry = zend_register_internal_class(&ce TSRMLS_CC);

	INIT_CLASS_ENTRY(ce2, "Callback", NULL);	
    callback_entry = zend_register_internal_class(&ce2 TSRMLS_CC);

    return SUCCESS;
}	

PHP_MSHUTDOWN_FUNCTION(event)
{
	event_entry = NULL;
	callback_entry = NULL;
	return SUCCESS;
}

zend_module_entry event_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	NULL,
	event_extname,
	event_functions,
	PHP_MINIT(event),
	PHP_MSHUTDOWN(event),
	NULL,
	NULL,
	NULL,
	event_extver,
	STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(event)