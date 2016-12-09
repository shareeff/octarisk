% setting attribute values
function obj = set(obj, varargin)
  % A) Specify fieldnames <-> types key/value pairs
  typestruct = struct(...
                'type', 'char' , ...
                'basis', 'numeric' , ...
                'scenario_mc', 'special' , ...
                'timestep_mc', 'special' , ...
                'scenario_stress', 'special' , ...
                'shift_type', 'numeric' , ...
                'name', 'char' , ...
                'id', 'char' , ...
                'description', 'char' , ...
                'model', 'char' , ...
                'mean', 'numeric' , ...
                'std', 'numeric' , ...
                'skew', 'numeric' , ...
                'kurt', 'numeric' , ...
                'mr_level', 'numeric' , ...
                'mr_rate', 'numeric' , ...
                'node2', 'numeric' , ...
                'node3', 'numeric' , ...
                'node', 'numeric' , ...
                'value_base', 'numeric' ...                
               );
  % B) store values in object
  if (length (varargin) < 2 || rem (length (varargin), 2) ~= 0)
    error ('set: expecting property/value pairs');
  end
  
  while (length (varargin) > 1)
    prop = varargin{1};
    prop = lower(prop);
    val = varargin{2};
    varargin(1:2) = [];
    % check, if property is an existing field
    if (sum(strcmpi(prop,fieldnames(typestruct)))==0)
        fprintf('set: not an allowed fieldname >>%s<< with value >>%s<< :\n',prop,any2str(val));
        fieldnames(typestruct)
        error ('set: invalid property of %s class: >>%s<<\n',class(obj),prop);
    end
    % get property type:
    type = typestruct.(prop);
    % input checks and validation
    retval = return_checked_input(obj,val,prop,type);
    % store property in object
    obj.(prop) = retval;
  end
end   