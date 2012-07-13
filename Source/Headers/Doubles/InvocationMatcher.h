#import "objc/runtime.h"
#import <vector>

namespace Cedar { namespace Doubles {

    class InvocationMatcher {
    public:
        typedef std::vector<Argument::shared_ptr_t> arguments_vector_t;
        enum { OBJC_DEFAULT_ARGUMENT_COUNT = 2 };

    public:
        InvocationMatcher(const SEL);
        virtual ~InvocationMatcher() {}

        void add_argument(const Argument::shared_ptr_t argument);
        template<typename T>
        void add_argument(const T &);

        bool matches_invocation(NSInvocation * const) const;
        NSString *mismatch_reason();

        const SEL & selector() const { return expectedSelector_; }
        const arguments_vector_t & arguments() const { return arguments_; }
        const bool match_any_arguments() const { return arguments_.empty(); }
        void verify_correct_number_of_arguments(id instance) const;

    private:
        bool matches_arguments(NSInvocation * const) const;

    private:
        const SEL expectedSelector_;
        arguments_vector_t arguments_;
        mutable NSString *mismatch_reason_;
    };

    inline InvocationMatcher::InvocationMatcher(const SEL selector) : expectedSelector_(selector) {
    }

    inline void InvocationMatcher::add_argument(const Argument::shared_ptr_t argument) {
        arguments_.push_back(argument);
    }

    template<typename T>
    inline void InvocationMatcher::add_argument(const T & value) {
        this->add_argument(Argument::shared_ptr_t(new TypedArgument<T>(value)));
    }

    inline bool InvocationMatcher::matches_invocation(NSInvocation * const invocation) const {
        return sel_isEqual(invocation.selector, expectedSelector_) && this->matches_arguments(invocation);
    }

    inline void InvocationMatcher::verify_correct_number_of_arguments(id instance) const {
        if (this->match_any_arguments()) {
            return true;
        }

        NSMethodSignature *methodSignature = [instance methodSignatureForSelector:this->selector()];
        size_t actualArgumentCount = [methodSignature numberOfArguments] - OBJC_DEFAULT_ARGUMENT_COUNT;
        size_t expectedArgumentCount = this->arguments().size();

        if (actualArgumentCount != expectedArgumentCount) {
            [[NSException exceptionWithName:NSInternalInconsistencyException
                                     reason:[NSString stringWithFormat:@"Wrong number of expected parameters for <%s>; expected: %d, actual: %d", this->selector(), expectedArgumentCount, actualArgumentCount]
                                   userInfo:nil]
             raise];
        }
    }

#pragma mark - Private interface
    inline bool InvocationMatcher::matches_arguments(NSInvocation * const invocation) const {
        bool matches = true;
        size_t index = 2;
        for (arguments_vector_t::const_iterator cit = arguments_.begin(); cit != arguments_.end() && matches; ++cit, ++index) {
            const char *actualArgumentEncoding = [invocation.methodSignature getArgumentTypeAtIndex:index];
            NSUInteger actualArgumentSize;
            NSGetSizeAndAlignment(actualArgumentEncoding, &actualArgumentSize, nil);

            char actualArgumentBytes[actualArgumentSize];
            [invocation getArgument:&actualArgumentBytes atIndex:index];
            matches = (*cit)->matches_bytes(&actualArgumentBytes);
        }
        return matches;
    }
}}
